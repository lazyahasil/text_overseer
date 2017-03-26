// std::copy() for char* in boost library(string.hpp) causes a MS complier error
#define _SCL_SECURE_NO_WARNINGS

#include "overseer.hpp"
#include "gui_main.hpp"
#include "misc_encoding.hpp"
#include "error_handler.hpp"
#include "overseer_rc.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/utility/string_view.hpp>

using namespace error_handler;
using namespace file_io;
using namespace nana;
using namespace overseer;
using namespace overseer_misc;

namespace overseer_gui
{
	nana::color line_num_default_color(unsigned int)
	{
		return nana::colors::antique_white;
	}

	AbstractBoxUnit::AbstractBoxUnit(window wd) : panel<false>(wd)
	{
		lab_name_.format(true);
		lab_name_.text_align(align::left, align_v::center);

		lab_name_.format(true);
		lab_name_.text_align(align::left, align_v::center);

		line_num_.bgcolor(colors::gray);

		_make_textbox_popup_menu();
	}

	void AbstractBoxUnit::_make_textbox_line_num() noexcept
	{
		drawing{ line_num_ }.draw([this](paint::graphics& graph) {
			const auto text_pos = this->textbox_.text_position();

			if (text_pos.empty())
				return;

			auto largest_num = text_pos.back().y + 1;
			unsigned int log10_num = 0;
			while ((largest_num /= 10) != 0)
				log10_num++;
			const unsigned int width = log10_num * 8 + 15;

			if (this->textbox_.edited() && width != graph.width())
			{
				std::stringstream ss;
				ss << "weight=" << width;
				this->place_.modify("line_num", ss.str().c_str());
				this->place_.collocate();
			}

			int top = this->textbox_.text_area().y;
			const unsigned int inner_width = width - 4;
			const unsigned int line_height = this->textbox_.line_pixels();

			for (const auto& pos : text_pos)
			{
				const auto num_wstr = std::to_wstring(pos.y + 1);
				const auto pixels = graph.text_extent_size(num_wstr).width;
				graph.rectangle({ 2, top, inner_width, line_height }, true, this->line_num_color_func_(pos.y));
				graph.string({ static_cast<int>(inner_width - pixels), top }, num_wstr);
				top += line_height;
			}
		});

		// nana::drawerbase::textbox::textbox_events event doesn't work at all (nana 1.4.1)

		textbox_.events().mouse_wheel([this] {
			this->refresh_textbox_line_num();
		}); // mouse wheel does effect even when it's not focused

		textbox_.events().resized([this] {
			this->refresh_textbox_line_num();
		});

		line_num_refresh_timer_.interval(k_ms_time_gui_timer_interval);
		line_num_refresh_timer_.elapse([this](const nana::arg_elapse&) {
			if (this->textbox_.focused())
				this->refresh_textbox_line_num();
		});
		line_num_refresh_timer_.start();
	}

	void AbstractBoxUnit::_make_textbox_popup_menu()
	{
		popup_menu_.append(u8"복사 (Ctrl+C)", [this](menu::item_proxy& ip) {
			if (!this->textbox_.selected())
				return;
			this->textbox_.copy();
		});

		popup_menu_.append(u8"붙여넣기 (Ctrl+V)", [this](menu::item_proxy& ip) {
			this->textbox_.paste();
		});

		popup_menu_.append_splitter();

		popup_menu_.append(u8"모두 선택 (Ctrl+A)", [this](menu::item_proxy& ip) {
			this->textbox_.select(true);
		});

		//textbox_.events().mouse_down(menu_popuper(popup_menu_));
		textbox_.events().mouse_down([this](const arg_mouse& arg) {
			this->textbox_.focus();
			menu_popuper(this->popup_menu_)(arg);
		});
	}

	AnswerTextBoxUnit::AnswerTextBoxUnit(window wd) : AbstractBoxUnit(wd)
	{
		place_.div(
			"<vert "
			"  <weight=25 margin=[0,0,3,0] lab_name>"
			"  <"
			"    <weight=15 line_num>"
			"    <weight=2>"
			"    <textbox>"
			"  >"
			"  <weight=42 margin=[3,0,0,0] lab_state>"
			">"
		);
		place_["lab_name"] << lab_name_;
		place_["line_num"] << line_num_;
		place_["textbox"] << textbox_;
		place_["lab_state"] << lab_state_;

		lab_name_.caption(u8"<size=11>정답 출력</>");
		lab_state_.caption(u8"<size=8>출력 파일을 읽을 때 자동으로 비교합니다. "
			u8"다시 읽기를 눌러도 됩니다. <bold>(실험 기능)</></>");
		lab_state_.format(true);

		_make_textbox_line_num();
	}

	AbstractIOFileBoxUnit::AbstractIOFileBoxUnit(window wd) : AbstractBoxUnit(wd)
	{
		// combo box order relys on FileIO::encoding
		combo_locale_.push_back(u8"자동");
		combo_locale_.push_back("ANSI");
		combo_locale_.push_back("UTF-8");
		combo_locale_.push_back(u8"서명 없는 UTF-8");
		combo_locale_.push_back("UTF-16LE");
		
		// widgets
		btn_reload_.enabled(false);

		// make event
		_make_events();
	}

	bool AbstractIOFileBoxUnit::update_label_state() noexcept
	{
		const auto file_is_changed = _check_last_write_time();

		if (file_is_changed)
		{
			bool did_read_file = false;

			for (auto i = 0; i < k_max_count_read_file; i++)
			{
				try
				{
					// read the file
					did_read_file = read_file();
				}
				catch (std::exception& e)
				{
					ErrorHdr::instance().report(
						ErrorHdr::priority::critical, 0,
						std::string("Unknown error in the file read function - ") + e.what(),
						wstr_to_utf8(file_.filename())
					);
				}
				if (did_read_file)
					break;
			}

			if (!did_read_file)
			{
				lab_state_.caption(u8"파일을 열지 못했습니다.");
				return false;
			}
		}
		else if (!last_write_time_is_vaild_)
		{
			lab_state_.caption(u8"파일을 찾지 못했습니다.");
			return false;
		}

		const auto term = std::chrono::system_clock::now() - last_write_time_;
		auto str = file_system::time_duration_to_string(
			std::chrono::duration_cast<std::chrono::seconds>(term),
			true,
			file_system::time_period_strings::k_korean_u8
		) + u8"전";
		lab_state_.caption(std::move(str));

		refresh_textbox_line_num();

		return file_is_changed;
	}

	bool AbstractIOFileBoxUnit::read_file()
	{
		std::unique_lock<std::mutex> lock(file_mutex_, std::try_to_lock);

		if (!lock)
			return false; // return if the file process is busy (fail trying to lock)

		if (!file_.open(std::ios::in | std::ios::binary))
		{
			ErrorHdr::instance().report(
				ErrorHdr::priority::info, 0, "Cannot open the file to read", wstr_to_utf8(file_.filename())
			);
			return false;
		}

		FileIOClosingGuard file_closer(file_);
		std::string str;

		try
		{
			str = file_.read_all();
		}
		catch (std::exception& e)
		{
			ErrorHdr::instance().report(
				ErrorHdr::priority::info, 0,
				std::string("Error while reading the file - ") + e.what(),
				wstr_to_utf8(file_.filename())
			);
			return false;
		}

		const auto locale = file_.locale();

		if (locale == FileIO::encoding::system) // ANSI
			textbox_.caption(charset(std::move(str)).to_bytes(unicode::utf8));
		else if (locale == FileIO::encoding::utf16_le) // UTF-16LE
			textbox_.caption(charset(std::move(str), unicode::utf16).to_bytes(unicode::utf8));
		else // UTF-8
			textbox_.caption(std::move(str));

		file_closer.close_safe(); // manual file close before unlock
		lock.unlock(); // manual unlock before nana::combox::option()

		// In mutex lock situation, it will cause nana gui exception(busy device).
		// Because, after nana::combox::option(), nana tries to call the event, which is stuck at deadlock.
		combo_locale_.option(static_cast<std::size_t>(locale));
		return true;
	}

	bool AbstractIOFileBoxUnit::_check_last_write_time() noexcept
	{
		if (file_.filename_wstring().empty())
			return false;

		boost::system::error_code ec;
		file_system::TimePointOfSys time_gotton;

		for (auto i = 0; i < k_max_count_check_last_file_write; i++)
		{
			time_gotton = file_system::file_last_write_time(file_.filename(), ec);
			if (!ec)
				break;
		}

		if ( last_write_time_ < time_gotton
			|| (!last_write_time_is_vaild_ && time_gotton.time_since_epoch().count() != 0LL) )
		{
			last_write_time_ = time_gotton;
			last_write_time_is_vaild_ = true;
			btn_reload_.enabled(true);
			return true;
		}

		if (ec)
		{
			if (last_write_time_is_vaild_)
			{
				ErrorHdr::instance().report(
					ErrorHdr::priority::info,
					ec.value(),
					std::string("Cannot check the last write time - ")
					+ charset(ec.message()).to_bytes(unicode::utf8),
					wstr_to_utf8(file_.filename())
				);
				last_write_time_is_vaild_ = false;
				btn_reload_.enabled(false);
			}
		}

		return false;
	}

	void AbstractIOFileBoxUnit::_make_events() noexcept
	{
		btn_reload_.events().click([this](const arg_click&) {
			this->read_file();
		});

		combo_locale_.events().selected([this](const arg_combox& arg_combo) {
			// update file locale
			for (auto i = 0; i < k_max_count_try_to_update_widget; i++)
			{
				std::unique_lock<std::mutex> lock(this->file_mutex_, std::try_to_lock); // to avoid deadlock
				if (lock)
				{
					file_.locale(static_cast<FileIO::encoding>(arg_combo.widget.option()));
					break;
				}
			}
		});
	}

	InputFileBoxUnit::InputFileBoxUnit(window wd) : AbstractIOFileBoxUnit(wd)
	{
		// div
		place_.div(
			"<vert "
			"  <weight=25 margin=[0,0,3,0]"
			"  <lab_name>"
			"    <weight=70 btn_reload>"
			//"    <weight=3>"
			//"    <weight=25 btn_folder>"
			"  >"
			"  <"
			"    <weight=15 line_num>"
			"    <weight=2>"
			"    <textbox>"
			"  >"
			"  <weight=42 margin=[3,0,0,0]"
			"    <vert margin=[0,3,0,0]"
			"      <margin=[0,0,3,0] lab_state>"
			"      < <> <weight=110 combo_locale> >"
			"    >"
			"    <weight=70 btn_save>"
			"  >"
			">");
		place_["lab_name"] << lab_name_;
		place_["btn_reload"] << btn_reload_;
		//place_["btn_folder"] << btn_folder_;
		place_["line_num"] << line_num_;
		place_["textbox"] << textbox_;
		place_["lab_state"] << lab_state_;
		place_["combo_locale"] << combo_locale_;
		place_["btn_save"] << btn_save_;

		// widget initiation - label
		lab_name_.caption(u8"<size=11><bold>input</>.txt</>");

		// make events
		btn_save_.events().click([this](const arg_click&) {
			if (!this->_write_file())
			{
				ErrorHdr::instance().report(
					ErrorHdr::priority::critical, 0,
					"The function of writing the file failed",
					wstr_to_utf8(file_.filename())
				);
				// open a message box
				msgbox mb(*this, u8"파일 쓰기 실패");
				mb.icon(msgbox::icon_error) << u8"파일 쓰기에 실패했습니다.\n" << file_.filename();
				mb.show();
			}
		});

		// etc.
		_make_textbox_line_num();
	}

	bool InputFileBoxUnit::read_file()
	{
		if (!AbstractIOFileBoxUnit::read_file()) // call its parent class's method
			return false;

		text_backup_u8_ = textbox_.caption();

		// in texts from nana the newline escapes are used as \n\r(LF CR)
		// => which causes a broken text file view in Windows; \r\n(CR LF) is normal
		std::size_t pos = 0;
		while ((pos = text_backup_u8_.find("\n\r", pos + 1)) != std::string::npos)
		{
			text_backup_u8_[pos] = L'\r';
			text_backup_u8_[++pos] = L'\n';
		}

		return true;
	}

	bool InputFileBoxUnit::_write_file()
	{
		std::unique_lock<std::mutex> lock(file_mutex_, std::try_to_lock);

		if (!lock)
			return false; // return if the file process is busy (fail trying to lock)

		if (!file_.open(std::ios::out | std::ios::binary))
		{
			ErrorHdr::instance().report(
				ErrorHdr::priority::critical, 0, "Cannot open the file to write", wstr_to_utf8(file_.filename())
			);
			msgbox mb(*this, u8"파일 쓰기 실패 (파일 열기)");
			mb.icon(msgbox::icon_error) << u8"파일 쓰기 도중 파일 열기에 실패했습니다.";
			mb.show();
			return false;
		}

		FileIOClosingGuard file_closer(file_);

		std::string buf;
		auto locale = file_.locale();

		if (locale == FileIO::encoding::unknown || locale == FileIO::encoding::system)
		{
			auto caption = textbox_.caption_wstring();
			try
			{
				buf = wstr_to_mstr(caption);
			}
			catch (std::range_error& e) // conversion fail on account of some unicode character
			{
				// substitute "\\n" for newline characters in caption string
				std::size_t pos = 0;
				while ((pos = caption.find(L"\n\r", pos + 1)) != std::string::npos)
				{
					caption[pos] = L'\\';
					caption[++pos] = L'n';
				}

				// report character conversion error
				ErrorHdr::instance().report(
					ErrorHdr::priority::critical, 0,
					std::string("Encoding conversion failed when writing the file (UTF-8 to ANSI) - ") + e.what(),
					wstr_to_utf8(caption)
				);

				// restore the file from backup
				_restore_opened_file_to_utf8();

				file_closer.close_safe(); // manual file close before unlock
				lock.unlock(); // manual unlock before nana::combox::option()
				// at the end of this function, there is a detailed explanation for the statement below
				combo_locale_.option(static_cast<std::size_t>(FileIO::encoding::utf8));

				// open a message box
				msgbox mb(*this, u8"파일 쓰기 실패 (인코딩 오류)");
				mb.icon(msgbox::icon_error);
				mb << u8"파일 쓰기 도중 변환할 수 없는 유니코드 문자가 발견되었습니다.\n";
				mb << u8"오류에 따른 파일 복구를 시도했습니다. (UTF-8)";
				mb.show();

				return false;
			}
		}
		else // UTF-8 or UTF-16LE
		{
			buf = textbox_.caption();
		}

		// in texts from nana the newline escapes are used as \n\r(LF CR)
		// => which causes a broken text file view in Windows; \r\n(CR LF) is normal
		std::size_t pos = 0;
		while ( (pos = buf.find("\n\r", pos + 1)) != std::string::npos )
		{
			buf[pos] = '\r';
			buf[++pos] = '\n';
		}

		std::u16string u16_buf;

		// charset(u8_str, unicode::utf8).to_bytes(unicode::utf16) is not good; better use std::wstring_convert
		if (locale == FileIO::encoding::utf16_le) // UTF-16LE
			u16_buf = utf8_to_utf16(buf);
		
		try
		{
			if (locale == FileIO::encoding::utf16_le)
				file_.write_all(u16_buf.data(), u16_buf.size() * 2);
			else
				file_.write_all(buf.data(), buf.size());
		}
		catch (std::exception& e)
		{
			// report error
			ErrorHdr::instance().report(
				ErrorHdr::priority::critical, 0,
				std::string("Error while writing the file - ") + e.what(), wstr_to_utf8(file_.filename())
			);

			// restore the file from backup
			_restore_opened_file_to_utf8();

			file_closer.close_safe(); // manual file close before unlock
			lock.unlock(); // manual unlock before nana::combox::option()
			combo_locale_.option(static_cast<std::size_t>(FileIO::encoding::utf8));

			// open a message box
			msgbox mb(*this, u8"파일 쓰기 실패");
			mb.icon(msgbox::icon_error);
			mb << u8"파일 쓰기 도중 직접적인 오류가 발생했습니다.\n";
			mb << u8"오류에 따른 파일 복구를 시도했습니다. (UTF-8)";
			mb.show();

			return false;
		}

		file_closer.close_safe(); // manual file close before unlock
		lock.unlock(); // manual unlock before nana::combox::option()

		// In mutex lock situation, it will cause nana gui exception(busy device).
		// Because, after nana::combox::option(), nana tries to call the event, which is stuck at deadlock.
		combo_locale_.option(static_cast<std::size_t>(locale));
		return true;
	}

	void InputFileBoxUnit::_restore_opened_file_to_utf8()
	{
		if (!file_.is_open() || text_backup_u8_.empty())
			return;
		file_.locale(FileIO::encoding::utf8);
		try
		{
			file_.write_all(text_backup_u8_.data(), text_backup_u8_.size());
		}
		catch (std::exception&)
		{
			// do nothing
		}
	}

	OutputFileBoxUnit::OutputFileBoxUnit(IOFilesTabPage& parent_tab_page) : AbstractIOFileBoxUnit(parent_tab_page)
	{
		// div
		place_.div(
			"<vert "
			"  <weight=25 margin=[0,0,3,0]"
			"  <lab_name>"
			"    <weight=70 btn_reload>"
			//"    <weight=3>"
			//"    <weight=25 btn_folder>"
			"  >"
			"  <"
			"    <weight=15 line_num>"
			"    <weight=2>"
			"    <textbox>"
			"  >"
			"  <weight=42 margin=[3,0,0,0]"
			"    <vert margin=[0,3,0,0]"
			"      <margin=[0,0,3,0] lab_state>"
			"      < <> <weight=110 combo_locale> >"
			"    >"
			"    <weight=70>"
			"  >"
			">");
		place_["lab_name"] << lab_name_;
		place_["btn_reload"] << btn_reload_;
		//place_["btn_folder"] << btn_folder_;
		place_["line_num"] << line_num_;
		place_["textbox"] << textbox_;
		place_["lab_state"] << lab_state_;
		place_["combo_locale"] << combo_locale_;

		// widget initiation - label
		lab_name_.caption(u8"<size=11><bold>output</>.txt</>");
		
		// widget initiation - textbox
		textbox_.editable(false);
		textbox_.enable_caret();

		// widget initiation - combox
		combo_locale_.enabled(false);

		// widget modification - menu
		popup_menu_.enabled(1, false); // make paste unable

		// etc.
		_make_textbox_line_num();
		line_num_color_func_ = [this](unsigned int num) {
			if (!this->did_line_diff_ || num >= this->line_diff_results_.size())
				return colors::antique_white;
			if (line_diff_results_[num])
				return colors::yellow_green;
			return colors::orange_red;
		};
		call_tab_page_line_diff_func_ = [&parent_tab_page] {
			return parent_tab_page.output_box_line_diff();
		};
	}

	bool OutputFileBoxUnit::read_file()
	{
		if (!AbstractIOFileBoxUnit::read_file()) // call its parent class's method
			return false;

		call_tab_page_line_diff_func_();
		return true;
	}

	OutputFileBoxUnit::line_diff_sign OutputFileBoxUnit::line_diff_between_answer(const std::string& answer)
	{
		// clear the result
		did_line_diff_ = false;
		line_diff_results_.clear();

		// nana::widget::caption returns string that newlined as "\n\r"
		const auto file_str = textbox_.caption();

		if (file_str.empty() || answer.empty())
			return line_diff_sign::error;

		using CIterRange = boost::iterator_range<std::string::const_iterator>;
		std::vector<CIterRange> f_lines, a_lines;
		std::string result;

		boost::iter_split(f_lines, file_str, boost::token_finder(boost::is_any_of("\n")));
		boost::iter_split(a_lines, answer, boost::token_finder(boost::is_any_of("\n")));

		const auto f_lines_size = f_lines.size();
		const auto a_lines_size = a_lines.size();
		std::size_t i, j;

		// loop for lines
		for (i = 0, j = 0; i < f_lines_size && j < a_lines_size; i++, j++)
		{
			if (f_lines[i].begin() == f_lines[i].end())
			{
				j--;
				continue;
			}
			if (f_lines[j].begin() == f_lines[j].end())
			{
				i--;
				continue;
			}

			std::vector<CIterRange> f_words, a_words;
			boost::iter_split(f_words, f_lines[i], boost::token_finder(boost::is_any_of(" ")));
			boost::iter_split(a_words, a_lines[i], boost::token_finder(boost::is_any_of(" ")));

			auto line_is_different = false;
			const auto f_words_size = f_words.size();
			const auto a_words_size = a_words.size();
			std::size_t k, l;

			// loop for words
			for (k = 0, l = 0; k < f_words_size && l < a_words_size; k++, l++)
			{
				boost::string_view f_sv, a_sv;

				if (f_words[k].size() == 0 && a_words[l].size() == 0) // prevent an infinite loop
					continue;

				if (f_words[k].size() != 0)
				{
					f_sv = boost::string_view(&*f_words[k].begin(), f_words[k].size());
				}
				else
				{
					l--;
					continue;
				}

				if (a_words[l].size() != 0)
				{
					a_sv = boost::string_view(&*a_words[l].begin(), a_words[l].size());
				}
				else
				{
					k--;
					continue;
				}

				if (f_sv != a_sv)
				{
					line_is_different = true;
					break;
				}
			}

			if (!line_is_different && k != f_words_size)
			{
				while (k < f_words_size)
				{
					if (f_words[k++].size() != 0)
					{
						line_is_different = true;
						break;
					}
				}
			}
			else if (!line_is_different && l != a_words_size)
			{
				while (l < a_words_size)
				{
					if (a_words[l++].size() != 0)
					{
						line_is_different = true;
						break;
					}
				}
			}
			
			if (line_is_different)
				line_diff_results_.push_back(false);
			else
				line_diff_results_.push_back(true);
		}

		auto f_rest_lines_size = f_lines_size - i;
		if (f_rest_lines_size && boost::ends_with(file_str, "\n\r"))
			f_rest_lines_size--;

		for (i = 0; i < f_rest_lines_size; i++)
			line_diff_results_.push_back(false);

		did_line_diff_ = true;

		if (f_lines_size < a_lines_size)
			return line_diff_sign::file_is_shorter;

		return line_diff_sign::done;
	}

	IOFilesTabPage::IOFilesTabPage(window wd) : panel<true>(wd)
	{
		place_.div(
			"<"
			"  <weight=5>"
			"  <"
			"    <weight=36% input_box>"
			"    | <weight=36% output_box>"
			"    | <answer_box>"
			"  >"
			"  <weight=5>"
			">"
		);
		place_["input_box"] << input_box_;
		place_["output_box"] << output_box_;
		place_["answer_box"] << answer_box_;
	}

	bool IOFilesTabPage::output_box_line_diff()
	{
		switch (output_box_.line_diff_between_answer(answer_box_.textbox_caption()))
		{
		case OutputFileBoxUnit::line_diff_sign::done:
			answer_box_.label_caption(u8"비교가 끝났습니다.");
			return true;
		case OutputFileBoxUnit::line_diff_sign::file_is_shorter:
			answer_box_.label_caption(u8"<red>파일이 더 짧습니다!!</>");
			return true;
		}
		return false;
	}

	MainWindow::MainWindow()
		: form(API::make_center(640, 400),
			appear::decorate<appear::sizable, appear::minimize>())
	{
		// set the form's title
		caption(std::string(u8"Text I/O File Overseer v") + k_version_str);
		// set minimum window size
		API::track_window_size(*this, { 590, 308 }, false);

		// div
		place_.div(
			"<vert "
			"  <weight=82 margin=[4, 3, 10, 3] "
			"    <weight=68 pic_logo>"
			"    <vert "
			"      <weight=18 margin=[0, 0, 2, 5] lab_title>"
			"      <margin=[0, 0, 4, 12] lab_welcome>"
			"    >"
			"    <weight=150 margin=[0, 0, 0, 3] btn_refresh>"
			"  >"
			"  <weight=25 tabbar>"
			"  <margin=[5, 0, 3, 0] tab_frame>"
			">"
		);
		place_["pic_logo"] << pic_logo_;
		place_["lab_title"] << lab_title_;
		place_["lab_welcome"] << lab_welcome_;
		place_["btn_refresh"] << btn_refresh_;
		place_["tabbar"] << tabbar_;
		place_.collocate();

		// widget initiation - picture
		paint::image img_logo;
		img_logo.open(overseer_rc::k_overseer_bmp, overseer_rc::k_overseer_bmp_size);
		pic_logo_.load(img_logo);
		pic_logo_.stretchable(false);
		pic_logo_.align(align::center, align_v::center);

		// widget initiation - label
		lab_title_.format(true);
		lab_welcome_.format(true);

		// initiation of tap pages
		tabbar_.toolbox(tabbar<std::string>::kits::scroll, true);
		tabbar_.toolbox(tabbar<std::string>::kits::list, true);
		_search_io_files();

		// make events and etc.
		_make_events();
		_make_timer_io_tab_state();
	}

	void MainWindow::_create_io_tab_page(
		std::string tab_name_u8,
		std::wstring input_filename,
		std::wstring output_filename
	) noexcept
	{
		auto page = std::make_shared<IOFilesTabPage>(*this);
		page->register_files(input_filename, output_filename);
		place_["tab_frame"].fasten(*page);
		tabbar_.push_back(std::move(tab_name_u8));
		const auto pos = io_tab_pages_.size();
		tabbar_.attach(pos, *page);
		tabbar_.tab_bgcolor(pos, colors::white);
		io_tab_pages_.push_back(std::move(page));
	}

	void MainWindow::_easter_egg_logo() noexcept
	{
		static bool was_called = false;

		if (!was_called) // first call
		{
			std::string str;

			// change lab_title_
			str = lab_title_.caption();
			str.replace(str.find(u8"감시기"), sizeof(u8"감시기") - 1, u8"<color=0x800080>감시기</>");
			lab_title_.caption(str);

			// change lab_welcome_
			str = lab_welcome_.caption();
			str.replace(str.find(u8"감시"), sizeof(u8"감시") - 1, u8"<bold color=0x800080>탐지</>");
			lab_welcome_.caption(str);

			// set flag to get this doing once
			was_called = true;
		}
		else // second call
		{
			// start debugging to a log file
			if (ErrorHdr::instance().is_started())
				return;

			msgbox mb(*this, u8"디버깅 시작", msgbox::yes_no);
			mb.icon(msgbox::icon_question) << u8"로그 파일에 디버깅 정보를 기록하시겠습니까?";

			if (mb() == msgbox::pick_yes)
			{
				// modify the form's title
				this->caption(this->caption() + " (debug mode)");

				// add a border to logo picture
				// bgcolor() doesn't work here (for example: pic_logo_.bgcolor(color_rgb(0x800080));)
				drawing dw(this->pic_logo_);
				dw.draw([](paint::graphics& graph) {
					graph.rectangle({ 0, 0, 68, 68 }, false, color_rgb(0x800080));
					graph.rectangle({ 1, 1, 66, 66 }, false, color_rgb(0x800080));
				});
				dw.update();
				
				// start debugging
				ErrorHdr::instance().start();
			}
		}
	}

	void MainWindow::_make_events() noexcept
	{
		// pic_logo_ is clicked => modify lab_title_ caption and start debugging
		pic_logo_.events().click([this](const arg_click&) {
			this->_easter_egg_logo();
		});

		// btn_refresh_ is clicked => _search_io_files()
		btn_refresh_.events().click([this](const arg_click&) {
			this->_search_io_files();
		});

		// trying to unload MainWindow => msgbox
		this->events().unload([this](const arg_unload& arg) {
			msgbox mb(*this, u8"프로그램 종료", msgbox::yes_no);
			mb.icon(msgbox::icon_question);
			mb << u8"정말로 종료하시겠습니까?";
			if (arg.cancel = (mb() == msgbox::pick_no))
				return;
			mb << u8"\n저장하지 않은 정보는 손실될 수 있습니다!\n\n종료하려면 '예'를 누르세요.";
			arg.cancel = (mb() == msgbox::pick_no);
		});
	}

	void MainWindow::_make_timer_io_tab_state() noexcept
	{
		timer_io_tab_state_.elapse([this] {
			const auto size = io_tab_pages().size();
			for (std::size_t i = 0; i < size; i++)
			{
				if (io_tab_pages()[i]->update_io_file_box_state())
					this->_make_timer_tabbar_color_animation(i);
			}
		});
		timer_io_tab_state_.interval(100);
		timer_io_tab_state_.start();
	}

	void MainWindow::_make_timer_tabbar_color_animation(std::size_t pos) noexcept
	{
		if (timers_tabbar_color_animation_.size() <= pos)
			timers_tabbar_color_animation_.resize(pos + 1);

		if (timers_tabbar_color_animation_[pos].timer_ptr)
			return;

		const auto func_color_level = [](unsigned int time) -> double {
			if (time >= 400 && time <= 1600)
				return 1.0 - static_cast<double>(time - 400) / 1200;
			return 1.0;
		};

		auto a_timer = std::make_shared<timer>();
		a_timer->interval(k_ms_time_gui_timer_interval);
		a_timer->elapse([this, pos, interval = a_timer->interval(), &func_color_level](const nana::arg_elapse&) {
			auto& time = this->timers_tabbar_color_animation_[pos].elapsed_time;
			this->tabbar_.tab_bgcolor(
				pos,
				color(color_rgb(0xff8c00)).blend(colors::white, func_color_level(time)) // #ff8c00 dark orange
			);
			time += interval;
			if (time >= 1600)
				this->_remove_timer_tabbar_color_animation(pos);
		});

		auto& timer_data = timers_tabbar_color_animation_[pos];
		timer_data.timer_ptr = std::move(a_timer);
		timer_data.elapsed_time = 0;
		timer_data.timer_ptr->start();
	}

	void MainWindow::_remove_timer_tabbar_color_animation(std::size_t pos) noexcept
	{
		auto& timer_data = timers_tabbar_color_animation_[pos];
		if (!timer_data.timer_ptr)
			return;
		timer_data.timer_ptr->stop();
		timer_data.timer_ptr.reset();
	}

	void MainWindow::_search_io_files() noexcept
	{
		// preserve the condition of timer_io_tab_state_
		const auto timer_io_tab_state_was_going = timer_io_tab_state_.started();
		timer_io_tab_state_.stop();

		// _remove_timer_tabbar_color_animation() for all timers_tabbar_color_animation_
		for (std::size_t i = 0; i < timers_tabbar_color_animation_.size(); i++)
			_remove_timer_tabbar_color_animation(i);

		auto pair_file_pairs_and_path_ecs = file_system::search_input_output_files(L"input.txt", L"output.txt");
		auto file_pairs = std::move(pair_file_pairs_and_path_ecs.first);
		const auto path_ecs = std::move(pair_file_pairs_and_path_ecs.second);

		// check error codes from file_system::search_input_output_files()
		for (const auto& path_ec : path_ecs)
		{
			const auto ec = path_ec.error_code();
			ErrorHdr::instance().report(
				ErrorHdr::priority::info,
				ec.value(),
				std::string("Cannot open the path while file search - ")
					+ charset(ec.message()).to_bytes(unicode::utf8),
				wstr_to_utf8(path_ec.path_str())
			);
		}

		// check if tab pages found already exists
		for (std::size_t i = 0; i < io_tab_pages_.size(); i++)
		{
			auto are_already_in_tabs = false;

			for (std::size_t j = 0; j < file_pairs.size(); j++)
			{
				if (io_tab_pages_[i]->is_same_files(file_pairs[j].first, file_pairs[j].second))
				{
					are_already_in_tabs = true;
					file_pairs.erase(file_pairs.begin() + j);
					break;
				}
			}

			if (are_already_in_tabs)
			{
				io_tab_pages_[i]->reload_files();
			}
			else
			{
				tabbar_.erase(i); // it will change current activated tab; but can't manage to handle this
				place_.erase(*io_tab_pages_[i]);
				io_tab_pages_.erase(io_tab_pages_.begin() + i--);
			}
		}

		// get the folder names and _create_io_tab_page()
		for (auto& file_pair : file_pairs)
		{
			const auto before_last_slash = file_pair.first.find_last_of(L"/\\") - 1;
			const auto after_second_last_slash = file_pair.first.find_last_of(L"/\\", before_last_slash) + 1;
			auto folder_wstr = file_pair.first.substr(
				after_second_last_slash,
				before_last_slash - after_second_last_slash + 1
			);
			if (folder_wstr.empty())
				folder_wstr = L"root";
			_create_io_tab_page(
				charset(std::move(folder_wstr)).to_bytes(unicode::utf8),
				std::move(file_pair.first),
				std::move(file_pair.second)
			);
		}

		// update nana::place
		place_.collocate();

		// restore the condition of timer_io_tab_state_
		if (timer_io_tab_state_was_going)
			timer_io_tab_state_.start();

		// _make_timer_tabbar_color_animation() for all tabs
		for (std::size_t i = 0; i < io_tab_pages_.size(); i++)
			_make_timer_tabbar_color_animation(i);
	}
}
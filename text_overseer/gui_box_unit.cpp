// std::copy() for char* in boost library(string.hpp) causes an error by MS compliers
#ifdef _MSC_VER
#define _SCL_SECURE_NO_WARNINGS
#endif

#include "gui.hpp"
#include "encoding.hpp"
#include "error_handler.hpp"

#include <iomanip>
#include <boost/algorithm/string.hpp>
#include <boost/utility/string_view.hpp>

using namespace nana;

namespace text_overseer
{
	using namespace error_handler;
	using namespace file_io;

	namespace gui
	{
		AbstractBoxUnit::AbstractBoxUnit(IOFilesTabPage& parent_tab_page)
			: panel<false>(parent_tab_page), tab_page_ptr_(&parent_tab_page)
		{
			lab_name_.format(true);
			lab_name_.text_align(align::left, align_v::center);

			lab_name_.format(true);
			lab_name_.text_align(align::left, align_v::center);

			line_num_.bgcolor(colors::gray);

			API::eat_tabstop(textbox_, false);

			_make_textbox_popup_menu();

			// make gui refresh timer
			gui_refresh_timer_.interval(k_ms_gui_timer_interval);
			gui_refresh_timer_.elapse([this](const nana::arg_elapse&) {
				if (this->textbox_.edited())
					this->_post_textbox_edited(true);
				if (this->textbox_.focused())
					this->refresh_textbox_line_num();
			});
			gui_refresh_timer_.start();
		}

		void AbstractBoxUnit::_make_textbox_line_num() noexcept
		{
			drawing{ line_num_ }.draw([this](paint::graphics& graph) {
				const auto text_pos = this->textbox_.text_position();

				// return if there's no text
				if (text_pos.empty())
					return;

				auto largest_num = text_pos.back().y + 1;
				unsigned int log10_num = static_cast<int>(std::log10(largest_num));
				const unsigned int width = log10_num * 8 + 15;

				// check whether the current width is suitable or not
				// (and if the parent tab page is not currently enabled(activated),
				//  an exception from std::vector<>::size() will be thrown to death in nana 1.4.1)
				const auto current_width = graph.width();
				if (this->tab_page_ptr_->enabled() && current_width != 0 && current_width != width)
				{
					std::stringstream ss;
					ss << "weight=" << width;
					this->place_.modify("line_num", ss.str().c_str());
					this->place_.collocate();
				}

				int top = this->textbox_.text_area().y;
				const unsigned int inner_width = width - 4;
				const unsigned int line_height = this->textbox_.line_pixels();

				// draw the line numbers
				for (const auto& pos : text_pos)
				{
					const auto num_wstr = std::to_wstring(pos.y + 1);
					const auto pixels = graph.text_extent_size(num_wstr).width;
					graph.rectangle({ 2, top, inner_width, line_height }, true, this->_line_num_color(pos.y));
					graph.string({ static_cast<int>(inner_width - pixels), top }, num_wstr);
					top += line_height;
				}
			});

			// nana::drawerbase::textbox::textbox_events event doesn't work at all (nana 1.4.1)

			textbox_.events().mouse_wheel([this] {
				this->refresh_textbox_line_num();
			}); // mouse wheel does effect(changes its line position) even when it's not focused

			textbox_.events().resized([this] {
				this->refresh_textbox_line_num();
			});
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

		AnswerTextBoxUnit::AnswerTextBoxUnit(IOFilesTabPage& parent_tab_page)
			: AbstractBoxUnit(parent_tab_page)
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
			lab_state_.format(true);

			_make_textbox_line_num();
		}

		color AnswerTextBoxUnit::_line_num_color(unsigned int num) noexcept
		{
			if (file_line_count_if_shorter_ != 0 && num >= file_line_count_if_shorter_)
				return colors::orange_red;
			return k_line_num_default_color;
		}

		void AnswerTextBoxUnit::_post_textbox_edited(bool is_edited) noexcept
		{
			if (is_edited)
			{
				tab_page_ptr_->output_box_line_diff();
				_reset_textbox_edited();
			}
		}

		AbstractIOFileBoxUnit::AbstractIOFileBoxUnit(IOFilesTabPage& parent_tab_page)
			: AbstractBoxUnit(parent_tab_page)
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
				false,
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

			_reset_textbox_edited();
			combo_locale_.option(static_cast<std::size_t>(locale)); // event won't happen because of mutex lock
			return true;
		}

		bool AbstractIOFileBoxUnit::is_same_file(const std::wstring& path_str) const noexcept
		{
			if (file_.filename_wstring() == path_str)
				return true;
			file_system::filesys::path own_path(file_.filename_wstring());
			file_system::filesys::path param_path(path_str);
			return own_path.compare(param_path) == 0;
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

			if (last_write_time_ < time_gotton
				|| (!last_write_time_is_vaild_ && time_gotton.time_since_epoch().count() != 0LL))
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
			btn_folder_.events().click([this](const arg_click&) {
#ifdef _WINDOWS
#endif
			});

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
						lock.unlock(); // InputFileBoxUnit::_post_textbox_edited() uses the mutex lock too
						this->_post_textbox_edited(true);
						break;
					}
				}
			});
		}

		InputFileBoxUnit::InputFileBoxUnit(IOFilesTabPage& parent_tab_page)
			: AbstractIOFileBoxUnit(parent_tab_page)
		{
			// div
			place_.div(
				"<vert "
				"  <weight=25 margin=[0,0,3,0]"
				"    <lab_name>"
				//"    <weight=30 margin=[0,3,0,2] btn_folder>"
				"    <weight=70 btn_reload>"
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

		void InputFileBoxUnit::_post_textbox_edited(bool is_edited) noexcept
		{
			std::unique_lock<std::mutex> lock(file_mutex_, std::try_to_lock);

			if (!lock)
				return; // return if the file process is busy (fail trying to lock)

			if (!did_post_edited_ && is_edited)
			{
				btn_save_.bgcolor(colors::orange);
				lab_name_.caption(lab_name_.caption() + &k_label_postfix_edited[0]);
				did_post_edited_ = true;
			}
		}

		void InputFileBoxUnit::_reset_textbox_edited() noexcept
		{
			// this function doesn't need mutex lock because it will be called from the read & write function
			AbstractIOFileBoxUnit::_reset_textbox_edited();
			btn_save_.bgcolor(colors::button_face);
			auto str = lab_name_.caption();
			const auto postfix_len = k_label_postfix_edited.size() - 1;
			const auto pos = str.rfind(&k_label_postfix_edited[0], std::string::npos, postfix_len);
			if (pos == std::string::npos)
				return;
			str.erase(pos, postfix_len);
			lab_name_.caption(std::move(str));
			did_post_edited_ = false;
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

					combo_locale_.option(static_cast<std::size_t>(FileIO::encoding::utf8));

					file_closer.close_safe(); // manual file close before unlock
					lock.unlock(); // manual unlock before nana::combox::option()

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
			while ((pos = buf.find("\n\r", pos + 1)) != std::string::npos)
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

				combo_locale_.option(static_cast<std::size_t>(FileIO::encoding::utf8));

				file_closer.close_safe(); // manual file close before unlock
				lock.unlock(); // manual unlock before nana::combox::option()

				// open a message box
				msgbox mb(*this, u8"파일 쓰기 실패");
				mb.icon(msgbox::icon_error);
				mb << u8"파일 쓰기 도중 직접적인 오류가 발생했습니다.\n";
				mb << u8"오류에 따른 파일 복구를 시도했습니다. (UTF-8)";
				mb.show();

				return false;
			}

			_reset_textbox_edited();
			combo_locale_.option(static_cast<std::size_t>(locale)); // its event won't happen because of the mutex lock
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

		OutputFileBoxUnit::OutputFileBoxUnit(IOFilesTabPage& parent_tab_page)
			: AbstractIOFileBoxUnit(parent_tab_page)
		{
			// div
			place_.div(
				"<vert "
				"  <weight=25 margin=[0,0,3,0]"
				"    <lab_name>"
				//"    <weight=30 margin=[0,3,0,2] btn_folder>"
				"    <weight=70 btn_reload>"
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
		}

		bool OutputFileBoxUnit::read_file()
		{
			if (!AbstractIOFileBoxUnit::read_file()) // call its parent class's method
				return false;

			tab_page_ptr_->output_box_line_diff();
			return true;
		}

		int OutputFileBoxUnit::line_diff_between_answer(const std::string& answer)
		{
			// clear the result
			did_line_diff_ = false;
			line_diff_results_.clear();

			// nana::widget::caption returns string that newlined as "\n\r"
			const auto file_str = textbox_.caption();

			if (file_str.empty() || answer.empty())
				return static_cast<int>(line_diff_sign::error);

			// using typename CIterRange
			using CIterRange = boost::iterator_range<std::string::const_iterator>;

			// token iter split for lines
			std::vector<CIterRange> f_lines, a_lines;
			boost::iter_split(f_lines, file_str, boost::token_finder(boost::is_any_of("\n")));
			boost::iter_split(a_lines, answer, boost::token_finder(boost::is_any_of("\n")));

			const auto f_lines_size = f_lines.size();
			const auto a_lines_size = a_lines.size();
			std::size_t i, j;

			// loop for lines
			for (i = 0, j = 0; i < f_lines_size && j < a_lines_size; i++, j++)
			{
				// ignore empty lines
				if (f_lines[i].begin() == f_lines[i].end())
				{
					if (a_lines[j].begin() != a_lines[j].end()) // prevent an infinite loop
						j--;
					continue;
				}
				if (a_lines[j].begin() == a_lines[j].end())
				{
					i--;
					continue;
				}

				// token iter split for words
				std::vector<CIterRange> f_words, a_words;
				boost::iter_split(f_words, f_lines[i], boost::token_finder(boost::is_any_of(" \t\r")));
				boost::iter_split(a_words, a_lines[i], boost::token_finder(boost::is_any_of(" \t\r")));

				const auto f_words_size = f_words.size();
				const auto a_words_size = a_words.size();
				std::size_t k, l;

				auto line_is_different = false;

				// loop for words
				for (k = 0, l = 0; k < f_words_size && l < a_words_size; k++, l++)
				{
					// check the sizes to prevent an exception when building boost::string_view objects
					if (f_words[k].size() == 0)
					{
						if (a_words[l].size() != 0) // prevent an infinite loop
							l--;
						continue;
					}
					if (a_words[l].size() == 0)
					{
						k--;
						continue;
					}

					boost::string_view f_sv(&*f_words[k].begin(), f_words[k].size());
					boost::string_view a_sv(&*a_words[l].begin(), a_words[l].size());

					// comparison of two boost::string_view objects
					if (f_sv != a_sv)
					{
						line_is_different = true;
						break;
					}
				}

				// check if there are remaining words not compared together
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

				// push the result to line_diff_results_
				if (line_is_different)
					line_diff_results_.push_back(false);
				else
					line_diff_results_.push_back(true);
			}

			// calculate the remaining lines in the file
			auto f_rest_lines_size = f_lines_size - i;
			// exception for a newline at the end of the file
			if (f_rest_lines_size && boost::ends_with(file_str, "\n\r"))
				f_rest_lines_size--;
			// push failed results for the remaining lines in the file
			for (i = 0; i < f_rest_lines_size; i++)
				line_diff_results_.push_back(false);

			did_line_diff_ = true;

			if (f_lines_size < a_lines_size)
				return f_lines_size;

			return static_cast<int>(line_diff_sign::done);
		}

		color OutputFileBoxUnit::_line_num_color(unsigned int num) noexcept
		{
			if (!did_line_diff_ || num >= line_diff_results_.size())
				return k_line_num_default_color;
			if (line_diff_results_[num])
				return colors::yellow_green;
			return colors::orange_red;
		}
	}
}
#include "gui.hpp"
#include "text_overseer.hpp"
#include "error_handler.hpp"
#include "rc_overseer.hpp"

#include <codecvt>
#include <thread>

using namespace nana;
using namespace text_overseer;

namespace gui
{
	AbstractBoxUnit::AbstractBoxUnit(nana::window wd) : nana::panel<false>(wd)
	{
		lab_name_.format(true);
		lab_name_.text_align(align::left, align_v::center);

		lab_name_.format(true);
		lab_name_.text_align(align::left, align_v::center);
	}

	TextBoxUnit::TextBoxUnit(window wd) : AbstractBoxUnit(wd)
	{
		place_.div(
			"<vert "
			"  <weight=25 margin=[0,0,3,0] lab_name>"
			"  <textbox>"
			"  <weight=42 margin=[3,0,0,0] lab_state>"
			">"
		);
		place_["lab_name"] << lab_name_;
		place_["textbox"] << textbox_;
		place_["lab_state"] << lab_state_;

		lab_name_.caption(u8"<size=11>정답 출력</>");
		lab_state_.caption(u8"텍스트 비교 기능 미완성");
	}

	AbstractIOFileBoxUnit::AbstractIOFileBoxUnit(nana::window wd) : AbstractBoxUnit(wd)
	{
		// combo box order relys on FileIO::encoding
		combo_locale_.push_back(u8"자동");
		combo_locale_.push_back("ANSI");
		combo_locale_.push_back("UTF-8");
		combo_locale_.push_back("UTF-16LE");
		
		// widgets
		btn_reload_.enabled(false);

		// make event
		_make_event_btn_reload();
		_make_event_combo_locale();
	}

	bool AbstractIOFileBoxUnit::update_label_state() noexcept
	{
		auto file_is_changed = _check_last_write_time();

		if (file_is_changed)
		{
			bool did_read_file = false;

			for (auto i = 0; i < k_max_read_file_count; i++)
			{
				try
				{
					did_read_file = _read_file();
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
				std::this_thread::sleep_for(std::chrono::milliseconds(100)); // wait 100ms
			}

			if (!did_read_file)
			{
				lab_state_.caption(u8"파일을 열지 못했습니다.");
				return false;
			}
			combo_locale_.option(static_cast<std::size_t>(file_.locale()));
				
		}
		else if (!last_write_time_is_vaild_)
		{
			lab_state_.caption(u8"파일을 찾지 못했습니다.");
			return false;
		}

		auto term = std::chrono::system_clock::now() - last_write_time_;
		auto str = file_system::time_duration_to_string(
			std::chrono::duration_cast<std::chrono::seconds>(term),
			true,
			file_system::time_period_strings::k_korean_u8
		);
		str += u8"전";
		lab_state_.caption(str);

		return file_is_changed;
	}

	bool AbstractIOFileBoxUnit::_read_file()
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

		std::string str;
		FileIO::encoding locale;

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
			file_.close();
			return false;
		}

		locale = file_.locale();

		if (locale == FileIO::encoding::system)
			textbox_.caption(charset(str).to_bytes(unicode::utf8));
		else if (locale == FileIO::encoding::utf8)
			textbox_.caption(str);
		else // UTF-16LE
			textbox_.caption(charset(str, unicode::utf16).to_bytes(unicode::utf8));

		file_.close();
		return true;
	}

	bool AbstractIOFileBoxUnit::_check_last_write_time() noexcept
	{
		if (*file_.filename() == '\0')
			return false;

		boost::system::error_code ec;
		file_system::TimePointOfSys time_gotton;

		for (auto i = 0; i < k_max_check_count_last_file_write; i++)
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
					text_overseer::wstr_to_utf8(file_.filename())
				);
				last_write_time_is_vaild_ = false;
				btn_reload_.enabled(false);
			}
		}

		return false;
	}

	void AbstractIOFileBoxUnit::_make_event_btn_reload() noexcept
	{
		btn_reload_.events().click([this](const arg_click&) {
			reload_file();
		});
	}

	void AbstractIOFileBoxUnit::_make_event_combo_locale() noexcept
	{
		combo_locale_.events().selected([this](const nana::arg_combox& arg_combo) {
			// update file locale
			for (auto i = 0; i < k_max_try_to_update_widget; i++)
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

	InputFileBoxUnit::InputFileBoxUnit(nana::window wd) : AbstractIOFileBoxUnit(wd)
	{
		place_.div(
			"<vert "
			"  <weight=25 margin=[0,0,3,0]"
			"  <lab_name>"
			"    <weight=70 btn_reload>"
			//"    <weight=3>"
			//"    <weight=25 btn_folder>"
			"  >"
			"  <textbox>"
			"  <weight=42 margin=[3,0,0,0]"
			"    <vert margin=[0,3,0,0]"
			"      <margin=[0,0,3,0] lab_state>"
			"      < <> <weight=100 combo_locale> >"
			"    >"
			"    <weight=80 btn_save>"
			"  >"
			">");
		place_["lab_name"] << lab_name_;
		place_["btn_reload"] << btn_reload_;
		//place_["btn_folder"] << btn_folder_;
		place_["textbox"] << textbox_;
		place_["lab_state"] << lab_state_;
		place_["combo_locale"] << combo_locale_;
		place_["btn_save"] << btn_save_;

		lab_name_.caption(u8"<size=11><bold>input</>.txt</>");

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
	}

	bool InputFileBoxUnit::_read_file()
	{
		if (!AbstractIOFileBoxUnit::_read_file())
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

		if (!file_.open(std::ios::out | std::ios::binary)) // MUST close and before return!!
		{
			ErrorHdr::instance().report(
				ErrorHdr::priority::critical, 0, "Cannot open the file to write", wstr_to_utf8(file_.filename())
			);
			msgbox mb(*this, u8"파일 쓰기 실패 (파일 열기)");
			mb.icon(msgbox::icon_error) << u8"파일 쓰기 도중 파일 열기에 실패했습니다.";
			mb.show();
			return false;
		}

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

				// open a message box
				msgbox mb(*this, u8"파일 쓰기 실패 (인코딩 오류)");
				mb.icon(msgbox::icon_error);
				mb << u8"파일 쓰기 도중 변환할 수 없는 유니코드 문자가 발견되었습니다.\n";
				mb << u8"오류에 따른 파일 복구를 시도했습니다.";
				mb.show();

				// post-return
				file_.close();
				lock.unlock(); // manual unlock
				combo_locale_.option(std::underlying_type_t<FileIO::encoding>(FileIO::encoding::utf8));
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
			u16_buf = utf16_to_utf8(buf);
		
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

			// open a message box
			msgbox mb(*this, u8"파일 쓰기 실패");
			mb.icon(msgbox::icon_error);
			mb << u8"파일 쓰기 도중 직접적인 오류가 발생했습니다.\n";
			mb << u8"오류에 따른 파일 복구를 시도했습니다.";
			mb.show();

			// post-return
			file_.close();
			lock.unlock(); // manual unlock
			combo_locale_.option(std::underlying_type_t<FileIO::encoding>(FileIO::encoding::utf8));
			return false;
		}

		// post-return
		file_.close();
		lock.unlock(); // manual unlock

		// In mutex lock situation, it will cause nana gui exception(busy device).
		// Because, after nana::combox::option(), nana tries to call the event, which is stuck at deadlock.
		combo_locale_.option(std::underlying_type_t<FileIO::encoding>(locale));
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

	OutputFileBoxUnit::OutputFileBoxUnit(nana::window wd) : AbstractIOFileBoxUnit(wd)
	{
		place_.div(
			"<vert "
			"  <weight=25 margin=[0,0,3,0]"
			"  <lab_name>"
			"    <weight=70 btn_reload>"
			//"    <weight=3>"
			//"    <weight=25 btn_folder>"
			"  >"
			"  <textbox>"
			"  <weight=42 margin=[3,0,0,0]"
			"    <vert margin=[0,3,0,0]"
			"      <margin=[0,0,3,0] lab_state>"
			"      < <> <weight=100 combo_locale> >"
			"    >"
			"    <weight=80>"
			"  >"
			">");
		place_["lab_name"] << lab_name_;
		place_["btn_reload"] << btn_reload_;
		//place_["btn_folder"] << btn_folder_;
		place_["textbox"] << textbox_;
		place_["lab_state"] << lab_state_;
		place_["combo_locale"] << combo_locale_;

		lab_name_.caption(u8"<size=11><bold>output</>.txt</>");

		combo_locale_.enabled(false);
	}

	IOFilesTabPage::IOFilesTabPage(window wd) : panel<true>(wd)
	{
		place_.div(
			"<"
			"  <weight=5>"
			"  <"
			"    <input_box>"
			"    | <output_box>"
			"    | <weight=25% answer_box>"
			"  >"
			"  <weight=5>"
			">"
		);
		place_["input_box"] << input_box_;
		place_["output_box"] << output_box_;
		place_["answer_box"] << answer_result_box_;
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

		// widget initiation - label
		paint::image img_logo;
		img_logo.open(k_overseer_bmp, k_overseer_bmp_size);
		pic_logo_.load(img_logo);
		pic_logo_.transparent(true);
		pic_logo_.stretchable(false);
		pic_logo_.align(align::center, align_v::center);

		// widget initiation - label
		lab_title_.format(true);
		lab_welcome_.format(true);

		// initiation of tap pages
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
		std::shared_ptr<IOFilesTabPage> page = std::make_shared<IOFilesTabPage>(*this);
		page->register_files(input_filename, output_filename);
		//->update_io_file_box_state();
		place_["tab_frame"].fasten(*page);
		tabbar_.push_back(std::move(tab_name_u8));
		auto pos = io_tab_pages_.size();
		tabbar_.attach(pos, *page);
		tabbar_.tab_bgcolor(pos, colors::white);
		io_tab_pages_.push_back(std::move(page));
	}

	void MainWindow::_make_events() noexcept
	{
		// pic_logo_ is clicked => modify lab_title_ caption and start debugging
		pic_logo_.events().click([this](const arg_click&) {

			static bool was_called = false;
			if (!was_called) // first call
			{
				std::string str;

				// change lab_title_
				str = this->lab_title_.caption();
				str.replace(
					str.find(u8"감시기"),
					sizeof(u8"감시기") - 1,
					u8"<color=0x800080>감시기</>"
				);
				this->lab_title_.caption(str);

				// change lab_welcome_
				str = this->lab_welcome_.caption();
				str.replace(
					str.find(u8"감시"),
					sizeof(u8"감시") - 1,
					u8"<bold color=0x800080>탐지</>"
				);
				this->lab_welcome_.caption(str);

				// set flag to get this doing once
				was_called = true;
			}
			else // second call
			{
				// start debugging to a log file
				if (!ErrorHdr::instance().is_started())
				{
					msgbox mb(*this, u8"디버깅 시작", msgbox::yes_no);
					mb.icon(msgbox::icon_question) << u8"로그 파일에 디버깅 정보를 기록하시겠습니까?";
					if (mb() == msgbox::pick_yes)
					{
						// modify the form's title
						this->caption(this->caption() + " (debug mode)");

						// add a border to logo picture
						drawing dw(*this);
						dw.draw([this](paint::graphics& graph) {
							point pos = this->pic_logo_.pos();
							graph.rectangle(
								rectangle(pos, nana::size(68, 68)), false, color_rgb(0x800080));
							graph.rectangle(
								rectangle(pos + point(1, 1), nana::size(66, 66)), false, color_rgb(0x800080));
						});
						dw.update();
						//this->pic_logo_.bgcolor(color_rgb(0x800080)); // bgcolor() doesn't work here
						ErrorHdr::instance().start();
					}
				}
			}

		});

		// btn_refresh_ is clicked => _search_io_files()
		btn_refresh_.events().click([this](const arg_click&) {
			this->_search_io_files();
		});

		// trying to unload MainWindow => msgbox
		this->events().unload([this](const arg_unload& arg) {
			msgbox mb(*this, u8"프로그램 종료", msgbox::yes_no);
			mb.icon(msgbox::icon_question) << u8"정말로 종료하시겠습니까?";
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
		const auto func_color_level = [](unsigned int time) -> double {
			if (time >= 400 && time <= 1600)
				return 1.0 - static_cast<double>(time - 400) / 1200;
			return 1.0;
		};

		if (timers_tabbar_color_animation_.size() <= pos)
			timers_tabbar_color_animation_.resize(pos + 1);

		if (timers_tabbar_color_animation_[pos].timer_ptr)
			return;

		auto timer_tca = std::make_shared<timer>(); // tca = tabbar_color_animation
		timer_tca->interval(20);
		timer_tca->elapse([this, index = pos, interval = timer_tca->interval(), &func_color_level]{
			auto& time = this->timers_tabbar_color_animation_[index].elapsed_time;
			this->tabbar_.tab_bgcolor(
				index,
				color(color_rgb(0xff8c00)).blend(colors::white, func_color_level(time)) // #ff8c00 dark orange
			);
			time += interval;
			if (time >= 1600)
				this->_remove_timer_tabbar_color_animation(index);
		});

		auto& timer_data = timers_tabbar_color_animation_[pos];
		timer_data.timer_ptr = std::move(timer_tca);
		timer_data.elapsed_time = 0U;

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
		auto timer_io_tab_state_was_going = timer_io_tab_state_.started();
		timer_io_tab_state_.stop();

		for (std::size_t i = 0U; i < timers_tabbar_color_animation_.size(); i++)
			_remove_timer_tabbar_color_animation(i);

		auto pair_file_pairs_and_path_ecs = file_system::search_input_output_files(L"input.txt", L"output.txt");
		auto file_pairs = std::move(pair_file_pairs_and_path_ecs.first);
		const auto path_ecs = std::move(pair_file_pairs_and_path_ecs.second);

		for (const auto& path_ec : path_ecs)
		{
			const auto ec = path_ec.error_code();
			ErrorHdr::instance().report(
				ErrorHdr::priority::info,
				ec.value(),
				std::string("Cannot open the path while file search - ")
					+ charset(ec.message()).to_bytes(unicode::utf8),
				text_overseer::wstr_to_utf8(path_ec.path_str())
			);
		}

		for (std::size_t i = 0; i < io_tab_pages_.size(); i++)
		{
			auto are_already_in_tabs = false;
			for (std::size_t j = 0; j < file_pairs.size(); j++)
			{
				if (io_tab_pages_[i]->is_same_files(file_pairs[j].first.c_str(), file_pairs[j].second.c_str()))
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
				io_tab_pages_.erase(io_tab_pages_.begin() + i--);
			}
		}

		for (auto& file_pair : file_pairs)
		{
			auto before_last_slash = file_pair.first.find_last_of(L"/\\") - 1;
			auto after_second_last_slash = file_pair.first.find_last_of(L"/\\", before_last_slash) + 1;
			auto folder_wstr = file_pair.first.substr(
				after_second_last_slash,
				before_last_slash - after_second_last_slash + 1
			);
			auto folder_u8 = charset(folder_wstr).to_bytes(unicode::utf8);
			if (folder_u8.empty())
				folder_u8 = "root";
			_create_io_tab_page(std::move(folder_u8), std::move(file_pair.first), std::move(file_pair.second));
		}

		place_.collocate();

		if (timer_io_tab_state_was_going)
			timer_io_tab_state_.start();

		for (std::size_t i = 0U; i < io_tab_pages_.size(); i++)
			_make_timer_tabbar_color_animation(i);
	}
}
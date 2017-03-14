#include "gui.hpp"
#include "error_handler.hpp"

#include <codecvt>

using namespace nana;

namespace text_overseer
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
			"  <weight=45 margin=[3,0,0,0] lab_state>"
			">"
		);
		place_["lab_name"] << lab_name_;
		place_["textbox"] << textbox_;
		place_["lab_state"] << lab_state_;

		lab_name_.caption(u8"<size=11>문제에서 제시한 출력</>");
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
		API::window_icon(btn_folder_, paint::image(R"(%PATH%\shell32.dll,4)"));

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
				did_read_file = _read_file();
				if (did_read_file)
					break;
			}

			if (did_read_file)
				combo_locale_.option(static_cast<std::size_t>(file_.locale()));
			else
				lab_state_.caption(u8"파일을 열지 못했습니다.");
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

	bool AbstractIOFileBoxUnit::_read_file() noexcept
	{
		std::lock_guard<std::mutex> g(file_mutex_);

		if (!file_.open(std::ios::in | std::ios::binary))
		{
			ErrorHdr::instance().report(
				ErrorHdr::priority::info, 0, "can't open file to read", file_.filename()
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
			ErrorHdr::instance().report(ErrorHdr::priority::info, 0, e.what());
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

		//#if _MSC_VER == 1900
		//		// Visual Studio bug: https://connect.microsoft.com/VisualStudio/feedback/details/1403302
		//		std::wstring_convert<std::codecvt_utf8_utf16<int16_t>, int16_t> converter;
		//		auto u16_ptr = reinterpret_cast<const int16_t*>(u16_str.data());
		//		textbox_.caption( converter.to_bytes(u16_ptr, u16_ptr + u16_str.size()) );
		//
		//#else
		//		std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
		//		textbox_.caption(converter.to_bytes(u16_str));
		//#endif
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

		if (last_write_time_ < time_gotton)
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
				ErrorHdr::instance().report(ErrorHdr::priority::info, ec.value(), ec.message().c_str());
				last_write_time_is_vaild_ = false;
				btn_reload_.enabled(false);
			}
		}

		return false;
	}

	void AbstractIOFileBoxUnit::_make_event_btn_reload() noexcept
	{
		btn_reload_.events().click([this](const arg_click&) {
			if (this->_read_file())
				combo_locale_.option(static_cast<std::size_t>(file_.locale()));
		});
	}

	void AbstractIOFileBoxUnit::_make_event_combo_locale() noexcept
	{
		combo_locale_.events().selected([this](const nana::arg_combox& arg_combo) {
			// update file locale
			std::lock_guard<std::mutex> g(file_mutex_);
			file_.locale(static_cast<FileIO::encoding>(arg_combo.widget.option()));
		});
	}

	InputFileBoxUnit::InputFileBoxUnit(nana::window wd) : AbstractIOFileBoxUnit(wd)
	{
		place_.div(
			"<vert "
			"  <weight=25 margin=[0,0,3,0]"
			"  <lab_name>"
			"    <weight=70 btn_reload>"
			"    <weight=3>"
			"    <weight=25 btn_folder>"
			"  >"
			"  <textbox>"
			"  <weight=45 margin=[3,0,0,0]"
			"    <vert "
			"      <lab_state>"
			"      < <> <weight=100 margin=[0,3,0,0] combo_locale> >"
			"    >"
			"    <weight=80 btn_save>"
			"  >"
			">");
		place_["lab_name"] << lab_name_;
		place_["btn_reload"] << btn_reload_;
		place_["btn_folder"] << btn_folder_;
		place_["textbox"] << textbox_;
		place_["lab_state"] << lab_state_;
		place_["combo_locale"] << combo_locale_;
		place_["btn_save"] << btn_save_;

		lab_name_.caption(u8"<size=11><bold>input</>.txt</>");

		btn_save_.events().click([this](const arg_click&) {
			this->_write_file();
		});
	}

	bool InputFileBoxUnit::_write_file() noexcept
	{
		std::lock_guard<std::mutex> g(file_mutex_);

		if (!file_.open(std::ios::out | std::ios::binary))
		{
			ErrorHdr::instance().report(
				ErrorHdr::priority::info, 0, "can't open file to write", file_.filename()
			);
			return false;
		}

		std::string buf;
		auto locale = file_.locale();

		if (locale == FileIO::encoding::unknown || locale == FileIO::encoding::system)
		{
			std::wstring wstr = textbox_.caption_wstring();
			using LocalFacet = DeletableFacet<std::codecvt_byname<wchar_t, char, std::mbstate_t>>;
			std::wstring_convert<LocalFacet> converter(new LocalFacet(""));
			buf = converter.to_bytes(wstr);
		}
		else if (locale == FileIO::encoding::utf8)
		{
			buf = textbox_.caption();
		}
		else // UTF-16LE
		{
			std::string u8_str = textbox_.caption();
			buf = charset(u8_str, unicode::utf8).to_bytes(unicode::utf16);
		}

		try
		{
			file_.write_all(buf, buf.size());
		}
		catch (std::exception& e)
		{
			ErrorHdr::instance().report(ErrorHdr::priority::info, 0, e.what());
			file_.close();
			return false;
		}

		file_.close();
		combo_locale_.option(std::underlying_type_t<FileIO::encoding>(locale));
		return true;
	}

	OutputFileBoxUnit::OutputFileBoxUnit(nana::window wd) : AbstractIOFileBoxUnit(wd)
	{
		place_.div(
			"<vert "
			"  <weight=25 margin=[0,0,3,0]"
			"  <lab_name>"
			"    <weight=70 btn_reload>"
			"    <weight=3>"
			"    <weight=25 btn_folder>"
			"  >"
			"  <textbox>"
			"  <weight=45 margin=[3,0,0,0]"
			"    <vert "
			"      <lab_state>"
			"      < <> <weight=100 margin=[0,3,0,0] combo_locale> >"
			"    >"
			"    <weight=80>"
			"  >"
			">");
		place_["lab_name"] << lab_name_;
		place_["btn_reload"] << btn_reload_;
		place_["btn_folder"] << btn_folder_;
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
		caption(std::string(u8"Text I/O File Overseer v") + VERSION_STRING);
		nana::API::track_window_size(*this, { 500, 240 }, false); //minimum window size

		// div
		place_.div(
			"<vert "
			"  <weight=80 margin=[4, 3, 10, 3] "
			"    <vert "
			"      <weight=18 margin=[0, 0, 2, 3] lab_title>"
			"      <margin=[0, 0, 2, 10] lab_welcome>"
			"    >"
			"    <weight=150 btn_refresh>"
			"  >"
			"  <weight=25 tabbar>"
			"  <margin=[5, 0, 3, 0] tab_frame>"
			">"
		);
		place_["lab_title"] << lab_title_;
		place_["lab_welcome"] << lab_welcome_;
		place_["btn_refresh"] << btn_refresh_;
		place_["tabbar"] << tabbar_;
		place_.collocate();

		// widget initiation
		lab_title_.format(true);
		lab_welcome_.format(true);

		// initiation of tap pages
		_search_io_files();

		// make events and etc.
		_make_events();
		_make_timer_io_tab_state();
	}

	void MainWindow::remove_timer_tabbar_color_animation(std::size_t pos) noexcept
	{
		auto& timer_data = timers_tabbar_color_animation_[pos];
		if (!timer_data.timer_ptr)
			return;
		timer_data.timer_ptr->stop();
		timer_data.timer_ptr.reset();
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
		btn_refresh_.events().click([this](const arg_click&) {
			this->_search_io_files();
		});

		this->events().unload([this](const arg_unload& ei) {
			msgbox mb(*this, u8"프로그램 종료", msgbox::yes_no);
			mb.icon(msgbox::icon_question) << u8"정말로 종료하시겠습니까?";
			ei.cancel = (mb() == msgbox::pick_no);
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
		auto factor = func_color_level(time);
		this->tabbar_.tab_bgcolor(
			index,
			color(0xff - static_cast<int>(factor * 0x14),
				0xff - static_cast<int>(factor * 0x9e),
				0xff - static_cast<int>(factor * 0xff))
		);
		time += interval;
		if (time >= 1600)
			this->remove_timer_tabbar_color_animation(index);
		});

		auto& timer_data = timers_tabbar_color_animation_[pos];
		timer_data.timer_ptr = std::move(timer_tca);
		timer_data.elapsed_time = 0U;

		timer_data.timer_ptr->start();
	}

	void MainWindow::_search_io_files() noexcept
	{
		auto timer_io_tab_state_was_going = timer_io_tab_state_.started();
		timer_io_tab_state_.stop();

		for (std::size_t i = 0U; i < timers_tabbar_color_animation_.size(); i++)
			remove_timer_tabbar_color_animation(i);

		auto pair_file_pairs_and_path_ecs = file_system::search_input_output_files(L"input.txt", L"output.txt");
		auto file_pairs = std::move(pair_file_pairs_and_path_ecs.first);
		const auto path_ecs = std::move(pair_file_pairs_and_path_ecs.second);

		for (const auto& path_ec : path_ecs)
		{
			const auto ec = path_ec.error_code();
			ErrorHdr::instance().report(
				ErrorHdr::priority::info, ec.value(), ec.message().c_str(), path_ec.path_str().c_str()
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
			if (!are_already_in_tabs)
			{
				tabbar_.erase(i);
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
	}
}
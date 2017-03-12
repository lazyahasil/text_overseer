#include "gui.hpp"
#include "error_handler.hpp"

#include <codecvt>

using namespace nana;

AbstractBoxUnit::AbstractBoxUnit(nana::window wd) : nana::panel<false>(wd)
{
	lab_name_.format(true);
	lab_name_.text_align(align::left, align_v::center);

	lab_state_.caption(u8"상태");
	lab_name_.format(true);
	lab_name_.text_align(align::left, align_v::center);
}

TextBoxUnit::TextBoxUnit(window wd) : AbstractBoxUnit(wd)
{
	place_.div(R"(<vert
				    <weight=25 margin=[0,0,3,0] lab_name>
				    <textbox>
				    <weight=45 margin=[3,0,0,0] lab_state>
				  >)");
	place_["lab_name"] << lab_name_;
	place_["textbox"] << textbox_;
	place_["lab_state"] << lab_state_;

	lab_name_.caption(u8"<size=11>문제에서 제시한 출력</>");
}

bool TextBoxUnit::update_label_state() noexcept
{
	return false;
}

AbstractIOFileBoxUnit::AbstractIOFileBoxUnit(nana::window wd) : AbstractBoxUnit(wd)
{
	// combo box order relys on FileIO::Locale
	combo_locale_.push_back(u8"자동");
	combo_locale_.push_back("ANSI");
	combo_locale_.push_back("UTF-8");
	combo_locale_.push_back("UTF-16LE");

	// make event
	_make_event_combo_locale();
}

bool AbstractIOFileBoxUnit::update_label_state() noexcept
{
	return false;
}

bool AbstractIOFileBoxUnit::register_file(const std::wstring& file_path) noexcept
{
	return false;
}

bool AbstractIOFileBoxUnit::read_file() noexcept
{
	std::lock_guard<std::mutex> g(file_mutex_);
	std::exception e;

	if (!file_.open(std::ios::in | std::ios::binary))
	{
		ErrorHandler::instance().report(
			ErrorHandler::Priority::info, 0, "can't open file to read: ", file_.filename().c_str()
		);
		return false;
	}
	
	std::string str = file_.read_all(e);
	if (*e.what() != '\0')
	{
		ErrorHandler::instance().report(ErrorHandler::Priority::info, 0, e.what());
		file_.close();
		return false;
	}
	auto locale = file_.locale();

	if (locale == FileIO::Locale::system)
		textbox_.caption(charset(str).to_bytes(unicode::utf8));
	else if (locale == FileIO::Locale::utf8)
		textbox_.caption(str);
	else // UTF-16LE
		textbox_.caption(charset(str, unicode::utf16).to_bytes(unicode::utf8));
	
	file_.close();
	combo_locale_.text(std::underlying_type_t<FileIO::Locale>(locale));
	return true;

	//#if _MSC_VER <= 1900 // Visual Studio bug: https://connect.microsoft.com/VisualStudio/feedback/details/1403302
	//		std::wstring_convert<std::codecvt_utf8_utf16<int16_t>, int16_t> converter;
	//		auto u16_ptr = reinterpret_cast<const int16_t*>(u16_str.data());
	//		textbox_.caption( converter.to_bytes(u16_ptr, u16_ptr + u16_str.size()) );
	//
	//#else
	//		std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
	//		textbox_.caption(converter.to_bytes(u16_str));
	//#endif
}

bool AbstractIOFileBoxUnit::check_last_write_time() const noexcept
{
	return false;
}

void AbstractIOFileBoxUnit::_make_event_combo_locale() noexcept
{
	combo_locale_.events().selected([this](const nana::arg_combox& arg_combo) mutable
	{
		// update file locale
		std::lock_guard<std::mutex> g(file_mutex_);
		file_.locale(static_cast<FileIO::Locale>(arg_combo.widget.option()));
	});
}

InputFileBoxUnit::InputFileBoxUnit(nana::window wd) : AbstractIOFileBoxUnit(wd)
{
	place_.div(R"(<vert
				    <weight=25 margin=[0,0,3,0]
					  <lab_name>
					  <weight=25 btn_folder>
					>
				    <textbox>
				    <weight=45 margin=[3,0,0,0]
					  <vert
						<lab_state>
						< <> <weight=100 margin=[0,3,0,0] combo_locale> >
					  >
					  <weight=80 btn_save>
					>
				  >)");
	place_["lab_name"] << lab_name_;
	place_["btn_folder"] << btn_folder_;
	place_["textbox"] << textbox_;
	place_["lab_state"] << lab_state_;
	place_["combo_locale"] << combo_locale_;
	place_["btn_save"] << btn_save_;

	lab_name_.caption(u8"<size=11><bold>input</>.txt</>");
}

bool InputFileBoxUnit::write_file() noexcept
{
	std::lock_guard<std::mutex> g(file_mutex_);
	std::exception e;

	if (!file_.open(std::ios::out | std::ios::binary))
	{
		ErrorHandler::instance().report(
			ErrorHandler::Priority::info, 0, "can't open file to write: ", file_.filename().c_str()
		);
		return false;
	}

	std::string buf;
	auto locale = file_.locale();

	if (locale == FileIO::Locale::system)
	{
		std::wstring wstr = textbox_.caption_wstring();
		std::wstring_convert<std::codecvt<wchar_t, char, std::mbstate_t>> converter;
		buf = converter.to_bytes(wstr);
	}
	else if (locale == FileIO::Locale::utf8)
	{
		buf = textbox_.caption();
	}
	else // UTF-16LE
	{
		std::string u8_str = textbox_.caption();
		buf = charset(u8_str, unicode::utf8).to_bytes(unicode::utf16);
	}

	file_.write_all(buf, buf.size(), e);
	if (*e.what() != '\0')
	{
		ErrorHandler::instance().report(ErrorHandler::Priority::info, 0, e.what());
		file_.close();
		return false;
	}

	file_.close();
	combo_locale_.text(std::underlying_type_t<FileIO::Locale>(locale));
	return true;
}

OutputFileBoxUnit::OutputFileBoxUnit(nana::window wd) : AbstractIOFileBoxUnit(wd)
{
	place_.div(R"(<vert
				    <weight=25 margin=[0,0,3,0]
					  <lab_name>
					  <weight=25 btn_folder>
					>
				    <textbox>
				    <weight=45 margin=[3,0,0,0]
					  <vert
						<lab_state>
						< <> <weight=100 margin=[0,3,0,0] combo_locale> >
					  >
					  <weight=80>
					>
				  >)");
	place_["lab_name"] << lab_name_;
	place_["btn_folder"] << btn_folder_;
	place_["textbox"] << textbox_;
	place_["lab_state"] << lab_state_;
	place_["combo_locale"] << combo_locale_;

	lab_name_.caption(u8"<size=11><bold>output</>.txt</>");
}

IOTextTapPage::IOTextTapPage(window wd) : panel<true>(wd)
{
	place_.div(R"(<
					<weight=5>
					<
					  <input_box>
					  | <output_box>
					  | <weight=25% answer_box>
					>
					<weight=5>
				  >)");
	place_["input_box"] << input_box_;
	place_["output_box"] << output_box_;
	place_["answer_box"] << answer_result_box_;

	label_.caption(
		file_system::time_duration_to_string(
			std::chrono::seconds(10), false, file_system::time_period_strings::k_english)
	);
}

MainWindow::MainWindow()
	: form(API::make_center(640, 400),
		appear::decorate<appear::sizable, appear::minimize>())
{
	caption(std::string(u8"입출력 텍스트 파일 감시기 v") + VERSION_STRING);

	// div
	place_.div(R"(<vert
					<weight=60 margin=[4, 3, 10, 3]
					  <lab_welcome>
					  <weight=150 btn_refresh>
					>
					<weight=25 tabbar>
					<margin=[5, 0, 3, 0] tab_frame>
				  >)");
	place_["lab_welcome"] << lab_welcome_;
	place_["btn_refresh"] << btn_refresh_;
	place_["tabbar"] << tabbar_;

	// widget initiation
	lab_welcome_.format(true);

	// initiation of tap pages
	tab_pages_.emplace_back(std::make_shared<IOTextTapPage>(*this));
	tab_pages_.emplace_back(std::make_shared<IOTextTapPage>(*this));
	tab_pages_.emplace_back(std::make_shared<IOTextTapPage>(*this));
	tabbar_.append(u8"테스트", *tab_pages_[0]);
	tabbar_.append(u8"테스트2", *tab_pages_[1]);
	tabbar_.append(u8"테스트3", *tab_pages_[2]);
	place_["tab_frame"].fasten(*tab_pages_[0])
		.fasten(*tab_pages_[1])
		.fasten(*tab_pages_[2]);
	tabbar_.tab_bgcolor(0, colors::yellow_green);
	tabbar_.tab_fgcolor(1, colors::chocolate);

	place_.collocate();

	// event
	this->events().unload([this](const arg_unload& ei)
	{
		msgbox mb(*this, u8"프로그램 종료", msgbox::yes_no);
		mb.icon(msgbox::icon_question) << u8"정말로 종료하시겠습니까?";
		ei.cancel = (mb() == msgbox::pick_no);
	});
}


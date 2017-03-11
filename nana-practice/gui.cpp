#include "gui.hpp"

using namespace nana;

TextBoxUnit::TextBoxUnit(window wd) : AbstractBoxUnit(wd)
{
	place_.div(R"(<vert
				    <weight=30 lab_name>
				    <textbox>
				    <weight=20 lab_state>
				  >)");
	place_["lab_name"] << lab_name_;
	place_["textbox"] << textbox_;
	place_["lab_state"] << lab_state_;

	lab_name_.caption(u8"<size=11>문제에서 제시한 출력</>");
	lab_name_.format(true);
	lab_name_.text_align(align::center, align_v::center);

	lab_state_.caption(u8"상태");
	lab_name_.format(true);
	lab_name_.text_align(align::left, align_v::center);
}

bool TextBoxUnit::update_label_state()
{
	return false;
}

bool AbstractIOFileBoxUnit::update_label_state()
{
	return false;
}

bool AbstractIOFileBoxUnit::register_file(const std::wstring& file_path, boost::system::error_code & ec)
{
	return false;
}

bool AbstractIOFileBoxUnit::read_file(boost::system::error_code & ec)
{
	return false;
}

bool AbstractIOFileBoxUnit::write_file(boost::system::error_code & ec)
{
	return false;
}

bool AbstractIOFileBoxUnit::check_last_write_time(boost::system::error_code & ec) const
{
	return false;
}

InputFileBoxUnit::InputFileBoxUnit(nana::window wd) : AbstractIOFileBoxUnit(wd)
{
	place_.div(R"(<vert
				    <weight=30 lab_name>
				    <textbox>
				    < weight=20 <lab_state> <but_save> >
				  >)");
	place_["lab_name"] << lab_name_;
	place_["textbox"] << textbox_;
	place_["lab_state"] << lab_state_;
	place_["but_save"] << but_save_;

	lab_name_.caption(u8"<size=11>문제에서 제시한 출력</>");
	lab_name_.format(true);
	lab_name_.text_align(align::center, align_v::center);

	lab_state_.caption(u8"상태");
	lab_name_.format(true);
	lab_name_.text_align(align::left, align_v::center);

	but_save_.caption(u8"저장");

	// test
	text_file_.set_filename(L"input2.txt");
	text_file_.open(std::ios::in | std::ios::binary);
	std::exception e;
	std::string str = text_file_.read_all(e);
	if (text_file_.file_locale() == file_io::LocaleEnum::system)
		charset(str).to_bytes(unicode::utf8);
	textbox_.caption(str);
	text_file_.close();
}

TapPageIOText::TapPageIOText(window wd) : panel<false>(wd)
{
	place_.div(R"(<
					<weight=5>
					<gap=5 boxes>
					<weight=5>
				  >)");
	place_["boxes"] << input_box_ << answer_result_box_;

	label_.caption(
		file_system::time_duration_to_string(
			std::chrono::seconds(10), false, file_system::time_period_strings::k_english)
	);
}

MainWindow::MainWindow() : form(API::make_center(600, 400), appear::decorate<appear::sizable>())
{
	caption(std::string(u8"입출력 텍스트 파일 감시기 v") + VERSION_STRING);

	lab_status_.format(false);
	lab_.format(true);

	place_.div("vert <weight=20 lab_status> <weight=40 label> <>"
		"<weight=20 tab> <weight=70% tab_frame>");
	place_["lab_status"] << lab_status_;
	place_["label"] << lab_;

	// initiation of tap pages
	place_["tab"] << tabbar_;
	place_["tab_frame"].fasten(tab_page_io_text_);

	tabbar_.append(u8"테스트", tab_page_io_text_);

	this->events().unload([this](const arg_unload& ei)
	{
		msgbox mb(*this, "Question", msgbox::yes_no);
		mb.icon(mb.icon_question) << "Are you sure you want to exit the demo?";
		ei.cancel = (mb() == mb.pick_no);
	});

	place_.collocate();
}
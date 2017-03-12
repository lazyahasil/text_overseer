#include "gui.hpp"

using namespace nana;

TextBoxUnit::TextBoxUnit(window wd) : AbstractBoxUnit(wd)
{
	place_.div(R"(<vert
				    <weight=25 margin=[0,0,3,0] lab_name>
				    <textbox>
				    <weight=50 margin=[3,0,0,0] lab_state>
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
				    <weight=25 margin=[0,0,3,0]
					  <lab_name>
					  <weight=25 btn_folder>
					>
				    <textbox>
				    <weight=50 margin=[3,0,0,0]
					  <lab_state>
					  <weight=80 btn_save>
					>
				  >)");
	place_["lab_name"] << lab_name_;
	place_["btn_folder"] << btn_folder_;
	place_["textbox"] << textbox_;
	place_["lab_state"] << lab_state_;
	place_["btn_save"] << btn_save_;

	lab_name_.caption(u8"<size=11><bold>input</>.txt</>");
	lab_name_.format(true);
	lab_name_.text_align(align::center, align_v::center);

	lab_state_.caption(u8"상태");
	lab_name_.format(true);
	lab_name_.text_align(align::left, align_v::center);
}

OutputFileBoxUnit::OutputFileBoxUnit(nana::window wd) : AbstractIOFileBoxUnit(wd)
{
	place_.div(R"(<vert
				    <weight=25 margin=[0,0,3,0]
					  <lab_name>
					  <weight=25 btn_folder>
					>
				    <textbox>
				    <weight=50 margin=[3,0,0,0] lab_state>
				  >)");
	place_["lab_name"] << lab_name_;
	place_["btn_folder"] << btn_folder_;
	place_["textbox"] << textbox_;
	place_["lab_state"] << lab_state_;

	lab_name_.caption(u8"<size=11><bold>output</>.txt</>");
	lab_name_.format(true);
	lab_name_.text_align(align::center, align_v::center);

	lab_state_.caption(u8"상태");
	lab_name_.format(true);
	lab_name_.text_align(align::left, align_v::center);
}

TapPageIOText::TapPageIOText(window wd) : panel<false>(wd)
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

MainWindow::MainWindow() : form(API::make_center(640, 400), appear::decorate<appear::sizable>())
{
	caption(std::string(u8"입출력 텍스트 파일 감시기 v") + VERSION_STRING);

	// div
	place_.div(R"(<vert
					<weight=60 margin=[4, 3, 10, 3]
					  <lab_welcome>
					  <weight=150 btn_refresh>
					>
					<weight=30 tab>
					<margin=[5, 0, 3, 0] tab_frame>
				  >)");
	place_["lab_welcome"] << lab_welcome_;
	place_["btn_refresh"] << btn_refresh_;
	// initiation of tap pages
	place_["tab"] << tabbar_;
	place_["tab_frame"].fasten(tab_page_io_text_);
	tabbar_.append(u8"테스트", tab_page_io_text_);
	// collocate
	place_.collocate();

	// widget initiation
	lab_welcome_.format(true);

	// event
	this->events().unload([this](const arg_unload& ei)
	{
		msgbox mb(*this, u8"프로그램 종료", msgbox::yes_no);
		mb.icon(mb.icon_question) << u8"정말로 종료하시겠습니까?";
		ei.cancel = (mb() == mb.pick_no);
	});
}
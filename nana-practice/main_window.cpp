#include "main_window.hpp"
#include "config.hpp"

#include <string>

using namespace nana;

MainWindow::MainWindow() : form(API::make_center(600, 400), appear::decorate<appear::sizable>())
{
	caption(std::string(u8"입출력 텍스트 파일 감시기 v") + VERSION_STRING);

	lab_status_.format(false);
	lab_.format(true);

	place_.div("vert <margin=5 lab_status> < <> <weight=500% label> <> arrange=[variable,100,variable]> <>"
		"<weight=20 tab> <tab_frame>");
	place_["lab_status"] << lab_status_;
	place_["label"] << lab_;

	init_tap_pages();

	this->events().unload([this](const arg_unload& ei)
	{
		msgbox mb(*this, ("Question"), msgbox::yes_no);
		mb.icon(mb.icon_question) << ("Are you sure you want to exit the demo?");
		ei.cancel = (mb() == mb.pick_no);
	});

	place_.collocate();
}

void MainWindow::init_tap_pages()
{
	place_["tab"] << tabbar_;
	place_["tab_frame"].fasten(tab_page_io_text_);

	tabbar_.append(u8"테스트", tab_page_io_text_);
}
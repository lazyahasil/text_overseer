#include "MainWindow.hpp"
#include "Config.hpp"

#include <string>

using namespace nana;

MainWindow::MainWindow()
{
	caption(std::string(u8"입출력 텍스트 파일 감시기 v") + VERSION_STRING);

	lab_status_.format(false);
	lab_.format(true);

	place_.div("vert <lab_status margin=5> < <> <weight=500% label> <> arrange=[variable,100,variable]> <>");
	place_["lab_status"] << lab_status_;
	place_["label"] << lab_;
	place_.collocate();
}
#include "gui.hpp"
#include "error_handler.hpp"

int main()
{
	text_overseer::gui::MainWindow window;

	window.show();

	nana::exec();

	return 0;
}
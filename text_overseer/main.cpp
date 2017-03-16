#include "gui.hpp"
#include "error_handler.hpp"

int main()
{
	gui::MainWindow window;

	window.show();

	nana::exec();

	return 0;
}
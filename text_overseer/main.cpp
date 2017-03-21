#include "gui.hpp"
#include "error_handler.hpp"

int main()
{
	overseer_gui::MainWindow window;

	window.show();

	nana::exec();

	return 0;
}
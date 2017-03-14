#include "gui.hpp"

int main()
{
	text_overseer::MainWindow window;

	window.show();

	nana::exec();

	return 0;
}
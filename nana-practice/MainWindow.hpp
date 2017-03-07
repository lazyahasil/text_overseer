#pragma once

#include <nana/gui.hpp>
#include <nana/gui/widgets/label.hpp>
#include <nana/gui/widgets/button.hpp>
#include <nana/gui/msgbox.hpp>
#include <nana/gui/place.hpp>

class MainWindow : public nana::form
{
public:
	MainWindow();

private:
	nana::place place_{ *this };
	nana::label lab_status_{ *this, u8"Nana C++ Library로 만들었습니다." };
	nana::label lab_{ *this, u8"test <bold red size=40>text</>" };
};
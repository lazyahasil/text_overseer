#pragma once

#include <nana/gui/widgets/label.hpp>
#include <nana/gui/widgets/panel.hpp>
#include <nana/gui/place.hpp>

class TapPageIOText : public nana::panel<false>
{
public:
	TapPageIOText(nana::window wd);

private:
	nana::place place_{ *this };
	nana::label label_{ *this, u8"테스트 레이블" };
};
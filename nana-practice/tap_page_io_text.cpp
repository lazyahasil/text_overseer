#include "tap_page_io_text.hpp"

TapPageIOText::TapPageIOText(nana::window wd) : panel<false>(wd)
{
	place_.div("<label>");
	place_["label"] << label_;

}
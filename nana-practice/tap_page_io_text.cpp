#include "tap_page_io_text.hpp"

#include "filesys_handler.hpp"

TapPageIOText::TapPageIOText(nana::window wd) : panel<false>(wd)
{
	place_.div("<label>");
	place_["label"] << label_;

	label_.caption(
		filesys_handler::time_duration_to_string(std::chrono::seconds(10), false, filesys_handler::time_period_strings::k_english)
	);
}
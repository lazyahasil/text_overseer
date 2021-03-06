﻿// std::copy() for char* in boost library(string.hpp) causes an error by MS compliers
#ifdef _MSC_VER
#define _SCL_SECURE_NO_WARNINGS
#endif

#include "gui.hpp"
#include "encoding.hpp"
#include "error_handler.hpp"
#include "resources.hpp"
#include "version.hpp"

#include <iomanip>

using namespace nana;

namespace text_overseer
{
	using namespace error_handler;
	using namespace file_io;

	namespace gui
	{
		IOFilesTabPage::IOFilesTabPage(window wd) : panel<true>(wd)
		{
			place_.div(
				"<"
				"  <margin=[7, 5, 3, 5] "
				"    <weight=36% input_box>"
				"    | <weight=36% output_box>"
				"    | <answer_box>"
				"  >"
				">"
			);
			place_["input_box"] << input_box_;
			place_["output_box"] << output_box_;
			place_["answer_box"] << answer_box_;
		}

		void IOFilesTabPage::output_box_line_diff()
		{
			const auto time_start = std::chrono::high_resolution_clock::now();
			const auto result = output_box_.line_diff_between_answer(answer_box_.textbox_caption());
			const auto time_end = std::chrono::high_resolution_clock::now();

			std::ostringstream oss;

			switch (result)
			{
			case static_cast<int>(OutputFileBoxUnit::line_diff_sign::error):
				answer_box_.label_caption(
					u8"<size=8>위에 정답 출력을 입력하면, 자동으로 출력 파일과 비교합니다.</>"
				);
				answer_box_.reset_line_count_of_file();
				return;
			case static_cast<int>(OutputFileBoxUnit::line_diff_sign::done):
				oss << u8"<green>비교가 끝났습니다.</>\n";
				answer_box_.reset_line_count_of_file();
				break;
			default:
				oss << u8"<red>파일이 더 짧습니다!</>\n";
				answer_box_.set_line_count_of_file(static_cast<std::size_t>(result));
			}

			// add the duration string
			const auto duration
				= std::chrono::duration_cast<std::chrono::microseconds>(time_end - time_start).count();
			oss << u8"걸린 시간: " << duration / 1000 << ".";
			oss << std::setw(3) << std::setfill('0') << duration % 1000 << u8" ms";
			answer_box_.label_caption(oss.str());

			// refresh line numbers
			output_box_.refresh_textbox_line_num();
			answer_box_.refresh_textbox_line_num();
		}

		WelcomeBox::WelcomeBox(MainWindow& parent_main_window)
			: panel<true>(parent_main_window), main_window_ptr_(&parent_main_window)
		{
			bgcolor(colors::white);

			// div
			place_.div(
				"<vert "
				"  <>"
				"  <weight=100 lab_welcome>"
				"  <weight=40"
				"    <> <weight=160 but_make_examples> <>"
				"  >"
				"  <>"
				">"
			);
			place_["lab_welcome"] << lab_welcome_;
			place_["but_make_examples"] << but_make_examples_;
			place_.collocate();

			// widget initiation
			lab_welcome_.format(true);
			lab_welcome_.text_align(align::center);
			lab_welcome_.bgcolor(colors::white);
			but_make_examples_.bgcolor(colors::alice_blue);

			// make events
			but_make_examples_.events().click([this](const arg_click&) {
				this->_make_examples();
			});
		}

		void WelcomeBox::_make_examples() noexcept
		{
			file_system::filesys::create_directory(L"example1");
			file_system::filesys::create_directory(L"example2");

			std::array<FileIO, 4> files{
				FileIO(L"example1/input.txt"),
				FileIO(L"example1/output.txt"),
				FileIO(L"example2/input.txt"),
				FileIO(L"example2/output.txt")
			};
			constexpr std::array<char*, 4> texts{
				"5\r\n1 2 3 4 5",
				"5\r\n5 4\r\n5 4 3\r\n5 4 3 2\r\n5 4 3 2 1\r\n",
				"(8+2*4)/4",
				"8 2 4 * + 4 /\r\n4\r\n"
			};
			
			try
			{
				for (std::size_t i = 0; i < 4; i++)
				{
					try
					{
						if (!files[i].open(std::ios::out | std::ios::binary))
							throw std::runtime_error("Cannot open the file to write");
						files[i].write_all(texts[i], std::strlen(texts[i]));
						files[i].close();
					}
					catch (std::exception& e)
					{
						throw std::runtime_error(
							std::string(e.what()) + " - " + wstr_to_utf8(files[i].filename_wstring())
						);
					}
				}
			}
			catch (std::runtime_error& e)
			{
				ErrorHdr::instance().report(
					ErrorHdr::priority::critical, 0, "Failed to write an example file", e.what()
				);
				msgbox mb(*main_window_ptr_, u8"예제 파일 만들기 실패");
				mb.icon(msgbox::icon_error) << u8"예제 파일을 쓰는 데 실패했습니다.";
				mb.show();
			}

			main_window_ptr_->search_io_files();
		}

		MainWindow::MainWindow()
			: form(API::make_center(640, 400), appear::decorate<appear::sizable, appear::minimize>())
		{
			// set the form's title
			caption(std::string(u8"Text I/O File Overseer v") + k_version_str);
			// set minimum window size
			API::track_window_size(*this, { 590, 308 }, false);

			// div
			place_.div(
				"<vert "
				"  <weight=82 margin=[4, 3, 10, 3] "
				"    <weight=68 pic_logo>"
				"    <vert "
				"      <weight=18 margin=[0, 0, 2, 5] lab_title>"
				"      <margin=[0, 0, 4, 12] lab_description>"
				"    >"
				"    <weight=150 margin=[0, 0, 0, 3] btn_refresh>"
				"  >"
				"  <weight=25 tabbar>"
				"  <tab_frame>"
				">"
			);
			place_["pic_logo"] << pic_logo_;
			place_["lab_title"] << lab_title_;
			place_["lab_description"] << lab_description_;
			place_["btn_refresh"] << btn_refresh_;
			place_["tabbar"] << tabbar_;
			place_.collocate();

			// widget initiation - picture
			paint::image img_logo;
			img_logo.open(&resources::k_overseer_bmp[0], resources::k_overseer_bmp.size());
			pic_logo_.load(img_logo);
			pic_logo_.stretchable(false);
			pic_logo_.align(align::center, align_v::center);

			// widget initiation - label
			lab_title_.format(true);
			lab_description_.format(true);

			// initiation of tap pages
			tabbar_.toolbox(tabbar<std::string>::kits::scroll, true);
			tabbar_.toolbox(tabbar<std::string>::kits::list, true);
			search_io_files();

			// make events and etc.
			_make_events();
			_make_timer_io_tab_state();
		}

		void MainWindow::search_io_files() noexcept
		{
			std::lock_guard<std::mutex> g(io_tab_mutex_);

			// preserve the condition of timer_io_tab_state_
			const auto timer_io_tab_state_was_going = timer_io_tab_state_.started();
			timer_io_tab_state_.stop();

			// _remove_tabbar_color_animation() for all tabbar_color_animations_
			for (std::size_t i = 0; i < tabbar_color_animations_.size(); i++)
				_remove_tabbar_color_animation(i);

			auto pair_file_pairs_and_path_ecs = file_system::search_input_output_files(L"input.txt", L"output.txt");
			auto file_pairs = std::move(pair_file_pairs_and_path_ecs.first);
			const auto path_ecs = std::move(pair_file_pairs_and_path_ecs.second);

			// check error codes from file_system::search_input_output_files()
			for (const auto& path_ec : path_ecs)
			{
				const auto ec = path_ec.error_code();
				ErrorHdr::instance().report(
					ErrorHdr::priority::info,
					ec.value(),
					std::string("Cannot open the path while file search - ")
					+ charset(ec.message()).to_bytes(unicode::utf8),
					wstr_to_utf8(path_ec.path_str())
				);
			}

			// check if tab pages found already exists
			for (std::size_t i = 0; i < io_tab_pages_.size(); i++)
			{
				auto are_already_in_tabs = false;

				for (std::size_t j = 0; j < file_pairs.size(); j++)
				{
					if (io_tab_pages_[i]->is_same_files(file_pairs[j].first, file_pairs[j].second))
					{
						are_already_in_tabs = true;
						file_pairs.erase(file_pairs.begin() + j);
						break;
					}
				}

				if (are_already_in_tabs)
				{
					io_tab_pages_[i]->reload_files();
				}
				else
				{
					tabbar_.erase(i); // it will change current activated tab; but can't manage to handle this
					place_.erase(*io_tab_pages_[i]);
					io_tab_pages_.erase(io_tab_pages_.begin() + i--);
				}
			}

			// get the folder names and _create_io_tab_page()
			for (auto& file_pair : file_pairs)
			{
				const auto before_last_slash = file_pair.first.find_last_of(L"/\\") - 1;
				const auto after_second_last_slash = file_pair.first.find_last_of(L"/\\", before_last_slash) + 1;
				auto folder_wstr = file_pair.first.substr(
					after_second_last_slash,
					before_last_slash - after_second_last_slash + 1
				);
				if (folder_wstr.empty())
					folder_wstr = L"root";
				_create_io_tab_page(
					charset(std::move(folder_wstr)).to_bytes(unicode::utf8),
					std::move(file_pair.first),
					std::move(file_pair.second)
				);
			}

			if (!io_tab_pages_.empty())
			{
				place_.erase(welcome_box_); // erase the welcome box
				welcome_box_.hide();
				// make io tab pages not enabled except the activated one
				_make_io_tabs_not_enabled_except_one(tabbar_.activated());
			}
			else
			{
				place_["tab_frame"].fasten(welcome_box_); // add the welcome box
				welcome_box_.show();
				API::refresh_window(tabbar_); // refresh the tabbar
			}

			// update nana::place
			place_.collocate();

			// restore the condition of timer_io_tab_state_
			if (timer_io_tab_state_was_going)
				timer_io_tab_state_.start();

			// _make_tabbar_color_animation() for all tabs
			for (std::size_t i = 0; i < io_tab_pages_.size(); i++)
				_make_tabbar_color_animation(i);
		}

		void MainWindow::_create_io_tab_page(
			std::string tab_name_u8,
			std::wstring input_filename,
			std::wstring output_filename
		) noexcept
		{
			// this function was made in the light of the nana example(widget_show.cpp)
			auto page = std::make_shared<IOFilesTabPage>(*this);
			page->register_files(input_filename, output_filename);
			place_["tab_frame"].fasten(*page);
			tabbar_.push_back(std::move(tab_name_u8));
			const auto pos = io_tab_pages_.size();
			tabbar_.attach(pos, *page);
			tabbar_.tab_bgcolor(pos, colors::white);
			io_tab_pages_.push_back(std::move(page));
		}

		void MainWindow::_easter_egg_logo() noexcept
		{
			static bool was_called = false;

			if (!was_called) // first call: change the labels
			{
				std::string str;

				// change lab_title_
				str = lab_title_.caption();
				str.replace(str.find(u8"감시기"), sizeof(u8"감시기") - 1, u8"<color=0x800080>감시기</>");
				lab_title_.caption(str);

				// change lab_welcome_
				str = lab_description_.caption();
				str.replace(str.find(u8"감시"), sizeof(u8"감시") - 1, u8"<bold color=0x800080>탐지</>");
				lab_description_.caption(str);

				// set flag to get this doing once
				was_called = true;
			}
			else // second call: start debugging to a log file
			{
				if (ErrorHdr::instance().is_started())
					return;

				msgbox mb(*this, u8"디버깅 시작", msgbox::yes_no);
				mb.icon(msgbox::icon_question) << u8"로그 파일에 디버깅 정보를 기록하시겠습니까?";

				if (mb() == msgbox::pick_yes)
				{
					// modify the form's title
					this->caption(this->caption() + " (debug mode)");

					// add a border to the logo picture
					// bgcolor() doesn't work here (for example: pic_logo_.bgcolor(color_rgb(0x800080));)
					drawing dw(this->pic_logo_);
					dw.draw([](paint::graphics& graph) {
						graph.rectangle({ 0, 0, 68, 68 }, false, color_rgb(0x800080));
						graph.rectangle({ 1, 1, 66, 66 }, false, color_rgb(0x800080));
					});
					dw.update();

					// start debugging
					ErrorHdr::instance().start();
				}
			}
		}

		void MainWindow::_make_events() noexcept
		{
			// pic_logo_ is clicked => modify lab_title_ caption and start debugging
			pic_logo_.events().click([this](const arg_click&) {
				this->_easter_egg_logo();
			});

			// btn_refresh_ is clicked => _search_io_files()
			btn_refresh_.events().click([this](const arg_click&) {
				this->search_io_files();
				// show a message box if empty
				if (io_tab_pages_.empty())
				{
					msgbox mb(*this, u8"검색 결과 없음");
					mb.icon(msgbox::icon_information) << u8"input.txt나 output.txt 파일을 찾지 못했습니다.";
					mb.show();
				}
			});

			tabbar_.events().activated([this](const arg_tabbar<std::string>& arg) {
				std::unique_lock<std::mutex> lock(io_tab_mutex_, std::try_to_lock);
				if (!lock)
					return;
				_make_io_tabs_not_enabled_except_one(arg.widget.activated());
			});

			// trying to unload MainWindow => msgbox
			this->events().unload([this](const arg_unload& arg) {
				msgbox mb(*this, u8"프로그램 종료", msgbox::yes_no);
				mb.icon(msgbox::icon_question);
				mb << u8"정말로 종료하시겠습니까?";
				if (arg.cancel = (mb() == msgbox::pick_no))
					return;
				if (!io_tab_pages_.empty())
				{
					mb << u8"\n저장하지 않은 정보는 손실될 수 있습니다!\n\n종료하려면 '예'를 누르세요.";
					arg.cancel = (mb() == msgbox::pick_no);
				}
			});
		}

		void MainWindow::_make_io_tabs_not_enabled_except_one(std::size_t pos) noexcept
		{
			const auto tab_n = tabbar_.length();
			for (std::size_t i = 0; i < tab_n; i++)
			{
				if (i == pos)
				{
					this->io_tab_pages_[i]->enabled(true);
					API::refresh_window_tree(this->io_tab_pages_[i]->handle());
				}
				else
				{
					this->io_tab_pages_[i]->enabled(false);
				}
			}
		}

		void MainWindow::_make_timer_io_tab_state() noexcept
		{
			timer_io_tab_state_.elapse([this] {
				const auto size = this->io_tab_pages_.size();
				for (std::size_t i = 0; i < size; i++)
				{
					if (this->io_tab_pages_[i]->update_io_file_box_state())
						this->_make_tabbar_color_animation(i);
				}
			});
			timer_io_tab_state_.interval(k_ms_update_label_state_interval);
			timer_io_tab_state_.start();
		}

		void MainWindow::_make_tabbar_color_animation(std::size_t pos) noexcept
		{
			if (tabbar_color_animations_.size() <= pos)
				tabbar_color_animations_.resize(pos + 1);

			if (tabbar_color_animations_[pos].timer_ptr)
				return;

			const auto func_color_level = [](unsigned int time) -> double {
				if (time >= 400 && time <= 1600)
					return 1.0 - static_cast<double>(time - 400) / 1200;
				return 1.0;
			};

			auto a_timer = std::make_shared<timer>();
			a_timer->interval(k_ms_gui_timer_interval);
			a_timer->elapse([this, pos, interval = a_timer->interval(), &func_color_level](const nana::arg_elapse&) {
				auto& time = this->tabbar_color_animations_[pos].elapsed_time;
				color color_leveled(color_rgb(0xff8c00)); // #ff8c00 dark orange
				color_leveled = color_leveled.blend(colors::button_face, func_color_level(time));
				this->tabbar_.tab_bgcolor(pos, color_leveled);
				time += interval;
				if (time >= 1600)
					this->_remove_tabbar_color_animation(pos);
			});

			auto& timer_data = tabbar_color_animations_[pos];
			timer_data.timer_ptr = std::move(a_timer);
			timer_data.elapsed_time = 0;
			timer_data.timer_ptr->start();
		}

		void MainWindow::_remove_tabbar_color_animation(std::size_t pos) noexcept
		{
			auto& timer_data = tabbar_color_animations_[pos];
			if (!timer_data.timer_ptr)
				return;
			timer_data.timer_ptr->stop();
			timer_data.timer_ptr.reset();
		}
	}
}
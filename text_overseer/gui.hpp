#pragma once

#include "file_system.hpp"
#include "file_io.hpp"

#include <array>
#include <mutex>
#include <nana/gui.hpp>
#include <nana/gui/msgbox.hpp>
#include <nana/gui/place.hpp>
#include <nana/gui/timer.hpp>
#include <nana/gui/widgets/combox.hpp>
#include <nana/gui/widgets/button.hpp>
#include <nana/gui/widgets/label.hpp>
#include <nana/gui/widgets/menu.hpp>
#include <nana/gui/widgets/panel.hpp>
#include <nana/gui/widgets/picture.hpp>
#include <nana/gui/widgets/tabbar.hpp>
#include <nana/gui/widgets/textbox.hpp>

namespace text_overseer
{
	namespace gui
	{
		constexpr int k_max_count_read_file = 3;
		constexpr int k_max_count_try_to_update_widget = 5;
		constexpr int k_max_count_check_last_file_write = 5;
		constexpr int k_ms_time_gui_timer_interval = 20;
		constexpr std::array<char, 24> k_label_postfix_edited{ " <color=0xff4500>(*)</>" };

		nana::color line_num_default_color(unsigned int);

		class IOFilesTabPage;

		class AbstractBoxUnit : public nana::panel<false>
		{
		public:
			AbstractBoxUnit(nana::window wd);
			virtual ~AbstractBoxUnit() = default;

			void refresh_textbox_line_num() noexcept
			{
				nana::API::refresh_window(line_num_);
			}

			virtual bool update_label_state() noexcept = 0;

		protected:
			virtual nana::color _line_num_color(unsigned int) noexcept { return nana::colors::antique_white; }
			void _make_textbox_line_num() noexcept;
			virtual void _post_textbox_edited(bool is_edited) noexcept { };

			virtual void _reset_textbox_edited() noexcept
			{
				refresh_textbox_line_num();
				textbox_.edited_reset();
			}

			nana::place place_{ *this };
			nana::label lab_name_{ *this };
			nana::label lab_state_{ *this, u8"상태" };
			nana::panel<true> line_num_{ *this };
			nana::textbox textbox_{ *this };
			nana::menu popup_menu_;

		private:
			void _make_textbox_popup_menu();

			nana::timer gui_refresh_timer_;
		};

		class AnswerTextBoxUnit : public AbstractBoxUnit
		{
		public:
			AnswerTextBoxUnit(IOFilesTabPage& parent_tab_page);

			bool update_label_state() noexcept override { return false; }
			void label_caption(const std::string &str) { lab_state_.caption(str); }
			void label_caption(std::string &&str) { lab_state_.caption(str); }
			std::string textbox_caption() { return textbox_.caption(); }

		protected:
			virtual void _post_textbox_edited(bool is_edited) noexcept override;

		private:
			IOFilesTabPage* tab_page_ptr_{ nullptr };
		};

		class AbstractIOFileBoxUnit : public AbstractBoxUnit
		{
		public:
			AbstractIOFileBoxUnit(nana::window wd);
			virtual ~AbstractIOFileBoxUnit() = default;

			virtual bool read_file();
			virtual bool update_label_state() noexcept override;
			bool is_same_file(const std::wstring& path_str) const noexcept;

			template <class StringT>
			void register_file(StringT&& file_path) noexcept
			{
				file_.filename(std::forward<StringT>(file_path));
			}

		protected:
			virtual bool _write_file() = 0;

			nana::button btn_reload_{ *this, u8"다시 읽기" };
			nana::button btn_folder_{ *this };
			nana::combox combo_locale_{ *this, u8"파일 인코딩" };

			file_io::FileIO file_;
			std::mutex file_mutex_;
			file_system::TimePointOfSys last_write_time_;
			bool last_write_time_is_vaild_{ false };

		private:
			bool _check_last_write_time() noexcept;
			void _make_events() noexcept;
		};

		class InputFileBoxUnit : public AbstractIOFileBoxUnit
		{
		public:
			InputFileBoxUnit(nana::window wd);

			virtual bool read_file() override;

		protected:
			virtual void _post_textbox_edited(bool is_edited) noexcept override;
			virtual void _reset_textbox_edited() noexcept override;
			virtual bool _write_file() override;

			nana::button btn_save_{ *this, u8"저장" };

		private:
			// call it when failed to write; the file_ must be opened for writing
			void _restore_opened_file_to_utf8();

			bool did_post_edited_{ false };
			std::string text_backup_u8_;
		};

		class OutputFileBoxUnit : public AbstractIOFileBoxUnit
		{
		public:
			explicit OutputFileBoxUnit(IOFilesTabPage& parent_tab_page);

			virtual bool read_file() override;

			enum class line_diff_sign
			{
				done,
				error,
				file_is_shorter
			};
			line_diff_sign line_diff_between_answer(const std::string& answer);

		protected:
			virtual nana::color _line_num_color(unsigned int num) noexcept override;
			virtual bool _write_file() noexcept override { return false; }

		private:
			void tab_page_line_diff_() noexcept;

			bool did_line_diff_{ false };
			std::vector<bool> line_diff_results_;
			IOFilesTabPage* tab_page_ptr_{ nullptr };
		};

		class IOFilesTabPage : public nana::panel<true>
		{
		public:
			IOFilesTabPage(nana::window wd);

			bool is_same_files(
				const std::wstring& input_filename,
				const std::wstring& output_filename
			) const noexcept
			{
				return input_box_.is_same_file(input_filename) && output_box_.is_same_file(output_filename);
			}

			bool output_box_line_diff();

			void register_files(std::wstring input_filename, std::wstring output_filename)
			{
				input_box_.register_file(input_filename);
				output_box_.register_file(output_filename);
			}

			void reload_files()
			{
				input_box_.read_file() || output_box_.read_file();
			}

			bool update_io_file_box_state() noexcept
			{
				auto input_state = input_box_.update_label_state();
				auto output_state = output_box_.update_label_state();
				return input_state || output_state;
			}

		protected:
			nana::place place_{ *this };

			InputFileBoxUnit input_box_{ *this };
			OutputFileBoxUnit output_box_{ *this };
			AnswerTextBoxUnit answer_box_{ *this };
		};

		class MainWindow : public nana::form
		{
		public:
			MainWindow();

			auto& io_tab_pages() noexcept { return io_tab_pages_; }

		private:
			void _create_io_tab_page(
				std::string tab_name_u8,
				std::wstring input_filename,
				std::wstring output_filename
			) noexcept;
			void _easter_egg_logo() noexcept;
			void _make_events() noexcept;
			void _make_timer_io_tab_state() noexcept;
			void _make_tabbar_color_animation(std::size_t pos) noexcept;
			void _remove_tabbar_color_animation(std::size_t pos) noexcept;
			void _search_io_files() noexcept;

			nana::place place_{ *this };
			nana::picture pic_logo_{ *this };
			nana::label lab_title_{
				*this,
				u8"<bold>텍스트 입출력 파일 감시기(Text Input/Output File Overseer)</>"
			};
			nana::label lab_welcome_{
				*this,
				u8"이 프로그램은 이 프로그램이 위치한 폴더와 하위 폴더에 있는 "
				u8"<blue>input.txt</>와 <blue>output.txt</>의 변동 여부를 실시간으로 감시합니다.\n"
				u8"사용한 라이브러리: <green size=10>Nana C++ GUI Library</>, <green size=10>Boost C++ Libraries</>"
			};
			nana::button btn_refresh_{ *this, u8"입출력 파일 다시 찾기" };

			nana::tabbar<std::string> tabbar_{ *this };
			std::vector<std::shared_ptr<IOFilesTabPage>> io_tab_pages_;

			nana::timer timer_io_tab_state_;

			struct TabbarColorAnimation
			{
				std::shared_ptr<nana::timer> timer_ptr;
				unsigned int elapsed_time;
			};
			std::vector<TabbarColorAnimation> tabbar_color_animations_;
		};
	}
}
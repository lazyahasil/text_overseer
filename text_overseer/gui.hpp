#pragma once

#include "file_system.hpp"
#include "file_io.hpp"

#include <mutex>
#include <nana/gui.hpp>
#include <nana/gui/msgbox.hpp>
#include <nana/gui/place.hpp>
#include <nana/gui/timer.hpp>
#include <nana/gui/widgets/combox.hpp>
#include <nana/gui/widgets/button.hpp>
#include <nana/gui/widgets/label.hpp>
#include <nana/gui/widgets/panel.hpp>
#include <nana/gui/widgets/picture.hpp>
#include <nana/gui/widgets/tabbar.hpp>
#include <nana/gui/widgets/textbox.hpp>

namespace overseer_gui
{
	constexpr int k_max_count_read_file = 3;
	constexpr int k_max_count_try_to_update_widget = 5;
	constexpr int k_max_count_check_last_file_write = 5;

	class AbstractBoxUnit : public nana::panel<false>
	{
	public:
		AbstractBoxUnit(nana::window wd);
		virtual ~AbstractBoxUnit() = default;

		virtual bool update_label_state() noexcept = 0;

	protected:
		nana::place place_{ *this };
		nana::label lab_name_{ *this };
		nana::label lab_state_{ *this, u8"상태" };
		nana::textbox textbox_{ *this };
	};

	class TextBoxUnit : public AbstractBoxUnit
	{
	public:
		TextBoxUnit(nana::window wd);

		bool update_label_state() noexcept override { return false; }

	private:
	};

	class AbstractIOFileBoxUnit : public AbstractBoxUnit
	{
	public:
		AbstractIOFileBoxUnit(nana::window wd);
		virtual ~AbstractIOFileBoxUnit() = default;

		virtual bool read_file();
		virtual bool update_label_state() noexcept override;

		bool is_same_file(const wchar_t* path_str) const noexcept
		{
			if (file_.filename_wstring().compare(path_str))
				return true;
			file_system::filesys::path own_path(file_.filename_wstring());
			return own_path.compare(path_str) == 0;
		}

		template <class StringT>
		void register_file(StringT&& file_path) noexcept
		{
			file_.filename(std::forward<StringT>(file_path));
		}

		decltype(auto) last_write_time() const noexcept { return file_system::TimePointOfSys(); }

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
		void _make_event_btn_reload() noexcept;
		void _make_event_combo_locale() noexcept;
	};

	class InputFileBoxUnit : public AbstractIOFileBoxUnit
	{
	public:
		InputFileBoxUnit(nana::window wd);

		virtual bool read_file() override;

	protected:
		virtual bool _write_file() override;

		nana::button btn_save_{ *this, u8"저장" };

	private:
		// call it when failed to write; the file must be opened for writing
		void _restore_opened_file_to_utf8();

		std::string text_backup_u8_;
	};

	class OutputFileBoxUnit : public AbstractIOFileBoxUnit
	{
	public:
		OutputFileBoxUnit(nana::window wd);

	protected:
		virtual bool _write_file() noexcept override { return false; }
	};

	class IOFilesTabPage : public nana::panel<true>
	{
	public:
		IOFilesTabPage(nana::window wd);

		bool is_same_files(const wchar_t* input_filename, const wchar_t* output_filename) const noexcept
		{
			return input_box_.is_same_file(input_filename) && output_box_.is_same_file(output_filename);
		}

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
			return input_box_.update_label_state() || output_box_.update_label_state();
		}

	protected:
		nana::place place_{ *this };

		InputFileBoxUnit input_box_{ *this };
		OutputFileBoxUnit output_box_{ *this };
		TextBoxUnit answer_result_box_{ *this };
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
		void _make_timer_tabbar_color_animation(std::size_t pos) noexcept;
		void _remove_timer_tabbar_color_animation(std::size_t pos) noexcept;
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
			u8"사용한 라이브러리: <green size=10>Nana C++ GUI Library</>, <green size=10>Boost.Filesystem</>"
		};
		nana::button btn_refresh_{ *this, u8"입출력 파일 다시 찾기" };

		nana::tabbar<std::string> tabbar_{ *this };
		std::vector<std::shared_ptr<IOFilesTabPage>> io_tab_pages_;

		nana::timer timer_io_tab_state_;

		struct TimersTabbarColorAnimationType
		{
			std::shared_ptr<nana::timer> timer_ptr;
			unsigned int elapsed_time;
		};
		std::vector<TimersTabbarColorAnimationType> timers_tabbar_color_animation_;
	};
}
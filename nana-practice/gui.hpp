#pragma once

#include "file_system.hpp"
#include "file_io.hpp"

#include <mutex>
#include <nana/gui.hpp>
#include <nana/gui/widgets/combox.hpp>
#include <nana/gui/widgets/button.hpp>
#include <nana/gui/widgets/label.hpp>
#include <nana/gui/widgets/panel.hpp>
#include <nana/gui/widgets/tabbar.hpp>
#include <nana/gui/widgets/textbox.hpp>
#include <nana/gui/msgbox.hpp>
#include <nana/gui/place.hpp>

#define VERSION_STRING "0.1"

class AbstractBoxUnit : public nana::panel<false>
{
public:
	AbstractBoxUnit(nana::window wd);
	virtual ~AbstractBoxUnit() = default;

	virtual bool update_label_state() noexcept = 0;

protected:
	nana::place place_{ *this };
	nana::label lab_name_{ *this };
	nana::label lab_state_{ *this };
	nana::textbox textbox_{ *this };
};

class TextBoxUnit : public AbstractBoxUnit
{
public:
	TextBoxUnit(nana::window wd);

	virtual bool update_label_state() noexcept override;

private:
};

class AbstractIOFileBoxUnit : public AbstractBoxUnit
{
public:
	AbstractIOFileBoxUnit(nana::window wd);
	virtual ~AbstractIOFileBoxUnit() = default;

	virtual bool update_label_state() noexcept override;

	bool register_file(const std::wstring& file_path) noexcept;
	bool read_file() noexcept;
	virtual bool write_file() noexcept = 0;

	decltype(auto) last_write_time() const noexcept { return file_system::TimePointOfSys(); }
	bool check_last_write_time() const noexcept;

protected:
	nana::button btn_folder_{ *this };
	nana::combox combo_locale_{ *this, u8"파일 인코딩" };

	FileIO file_;
	std::mutex file_mutex_;
	file_system::TimePointOfSys last_write_time_;

private:
	void _make_event_combo_locale() noexcept;
};

class InputFileBoxUnit : public AbstractIOFileBoxUnit
{
public:
	InputFileBoxUnit(nana::window wd);

	virtual bool write_file() noexcept override;

private:
	nana::button btn_save_{ *this, u8"저장" };
};

class OutputFileBoxUnit : public AbstractIOFileBoxUnit
{
public:
	OutputFileBoxUnit(nana::window wd);

	virtual bool write_file() noexcept override { return false; }

private:
	
};

class IOTextTapPage : public nana::panel<true>
{
public:
	IOTextTapPage(nana::window wd);

private:
	nana::place place_{ *this };

	InputFileBoxUnit input_box_{ *this };
	OutputFileBoxUnit output_box_{ *this };
	TextBoxUnit answer_result_box_{ *this };

	nana::label label_{ *this, u8"테스트 레이블" };
};

class MainWindow : public nana::form
{
public:
	MainWindow();

private:
	nana::place place_{ *this };
	nana::label lab_welcome_{
		*this,
		u8"이 프로그램은 이 프로그램이 위치한 폴더와 하위 폴더에 있는 "
		u8"<blue>input.txt</>와 <blue>output.txt</>의 변동 여부를 실시간으로 감지합니다.\n"
		u8"사용한 라이브러리: <green size=10>Nana C++ GUI Library</>, <green size=10>Boost.Filesystem</>"
	};
	nana::button btn_refresh_{ *this, u8"입출력 파일 다시 찾기" };
	nana::tabbar<std::string> tabbar_{ *this };
	std::vector<std::shared_ptr<nana::panel<true>>> tab_pages_;
};
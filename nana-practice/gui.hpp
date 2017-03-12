﻿#pragma once

#include "file_system.hpp"
#include "file_io.hpp"

#include <nana/gui.hpp>
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
	AbstractBoxUnit(nana::window wd) : nana::panel<false>(wd) { }
	virtual ~AbstractBoxUnit() { }

	virtual bool update_label_state() = 0;

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

	virtual bool update_label_state() override;

private:
};

class AbstractIOFileBoxUnit : public AbstractBoxUnit
{
public:
	AbstractIOFileBoxUnit(nana::window wd) : AbstractBoxUnit(wd) { }
	virtual ~AbstractIOFileBoxUnit() { }

	virtual bool update_label_state() override;

	bool register_file(const std::wstring& file_path, boost::system::error_code& ec);
	bool read_file(boost::system::error_code& ec);
	bool write_file(boost::system::error_code& ec);

	decltype(auto) last_write_time() const { return file_system::TimePointOfSys(); }
	bool check_last_write_time(boost::system::error_code& ec) const;

protected:
	nana::button btn_folder_{ *this };

	file_io::IOTextFile text_file_;
	file_system::TimePointOfSys last_write_time_;
};

class InputFileBoxUnit : public AbstractIOFileBoxUnit
{
public:
	InputFileBoxUnit(nana::window wd);

private:
	nana::button btn_save_{ *this, u8"저장" };
};

class OutputFileBoxUnit : public AbstractIOFileBoxUnit
{
public:
	OutputFileBoxUnit(nana::window wd);

private:
	
};

class TapPageIOText : public nana::panel<false>
{
public:
	TapPageIOText(nana::window wd);

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
		u8"<green bold size=10>Nana C++ GUI Library</>로 만들었습니다.\n"
		u8"이 프로그램이 위치한 폴더와 하위 폴더에 있는 "
		u8"<blue>input.txt</>와 <blue>output.txt</>의 변동 여부를 실시간으로 감지합니다."
	};
	nana::button btn_refresh_{ *this, u8"입출력 파일 다시 찾기" };
	nana::tabbar<std::string> tabbar_{ *this };
	TapPageIOText tab_page_io_text_{ *this };
	// std::vector<std::shared_ptr<nana::panel<false>>> tab_pages_io_text_;
};
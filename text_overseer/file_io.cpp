﻿#include "file_io.hpp"

#include <array>
#include <vector>

bool FileIO::open(std::ios::openmode mode) // needs std::ios::binary
{
	if (filename_.empty() || file_.is_open())
		return false;
	file_openmode_ = mode;
	file_.open(filename_, file_openmode_);
	return file_.good();
}

void FileIO::locale(encoding locale) noexcept
{
	if (locale == encoding::unknown)
		file_locale_ = encoding::system;
	else
		file_locale_ = locale;
}

FileIO::encoding FileIO::read_bom()
{
	std::array<unsigned char, 3> buf;
	if (!_read_file_check())
		return encoding::unknown;
	file_.seekg(0, std::ios::end);
	auto file_size = file_.tellg();
	file_.seekg(0, std::ios::beg);
	if (file_size > 1)
	{
		file_.read(&buf[0], 2);
		if (buf[0] == bom::k_u16_le[0] && buf[1] == bom::k_u16_le[1])
			return encoding::utf16_le;
		if (file_size > 2)
		{
			file_.read(&buf[2], 1);
			if (buf[0] == bom::k_u8[0] && buf[1] == bom::k_u8[1] && buf[2] == bom::k_u8[2])
				return encoding::utf8;
		}
	}
	file_.seekg(0, std::ios::beg);
	return encoding::system;
}

bool FileIO::update_locale_by_read_bom()
{
	auto locale = read_bom();
	if (locale == encoding::unknown)
	{
		// UTF-8 without BOM detection needed (not implemented yet)
		file_locale_ = encoding::system;
		return false;
	}
	file_locale_ = locale;
	return true;
}

std::string FileIO::read_all()
{
	std::string buf;
	read_all(buf, 0U, true);
	return buf;
}

std::u16string FileIO::read_all_u16()
{
	std::u16string buf;
	read_all(buf, 0U, true);
	return buf;
}

bool FileIO::_read_file_check()
{
	if (!file_)
		return false;
	if ((file_openmode_ & std::ios::in) == false)
		throw std::runtime_error("file stream is not input mode");
	else if ((file_openmode_ & std::ios::binary) == false)
		throw std::runtime_error("file stream is not binary mode");
	else
		return true;
	return false;
}

bool FileIO::_write_file_check()
{
	if (!file_)
		return false;
	else if ((file_openmode_ & std::ios::out) == false)
		throw std::runtime_error("file stream is not output mode");
	else if ((file_openmode_ & std::ios::binary) == false)
		throw std::runtime_error("file stream is not binary mode");
	else
		return true;
	return false;
}
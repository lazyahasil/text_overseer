#include "file_io.hpp"

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

void FileIO::locale(Locale locale) noexcept
{
	if (locale == Locale::unknown)
		file_locale_ = Locale::system;
	else
		file_locale_ = locale;
}

FileIO::Locale FileIO::read_bom(std::exception& e) noexcept
{
	std::array<unsigned char, 3> buf;
	if (!_read_file_check(e))
		return Locale::unknown;
	try
	{
		file_.seekg(0, std::ios::end);
		auto file_size = file_.tellg();
		file_.seekg(0, std::ios::beg);
		if (file_size > 1)
		{
			file_.read(&buf[0], 2);
			if (buf[0] == bom::k_u16_le[0] && buf[1] == bom::k_u16_le[1])
				return Locale::utf16_le;
			if (file_size > 2)
			{
				file_.read(&buf[2], 1);
				if (buf[0] == bom::k_u8[0] && buf[1] == bom::k_u8[1] && buf[2] == bom::k_u8[2])
					return Locale::utf8;
			}
		}
		file_.seekg(0, std::ios::beg);
	}
	catch (std::exception& _e)
	{
		e = _e;
	}
	return Locale::system;
}

bool FileIO::update_locale_by_read_bom(std::exception& e) noexcept
{
	auto locale = read_bom(e);
	if (locale == Locale::unknown)
	{
		if (*e.what() == '\0')
			file_locale_ = Locale::system;
		return false;
	}
	file_locale_ = locale;
	return true;
}

std::string FileIO::read_all(std::exception& e) noexcept
{
	std::string buf;
	read_all(buf, 0U, true, e);
	return buf;
}

std::u16string FileIO::read_all_u16(std::exception& e) noexcept
{
	std::u16string buf;
	read_all(buf, 0U, true, e);
	return buf;
}

bool FileIO::_read_file_check(std::exception& e) noexcept
{
	auto result = false;
	if (!file_)
		e = std::runtime_error("invaild file stream");
	else if ((file_openmode_ & std::ios::in) == false)
		e = std::runtime_error("file stream is not input mode");
	else if ((file_openmode_ & std::ios::binary) == false)
		e = std::runtime_error("file stream is not binary mode");
	else
		result = true;
	return result;
}

bool FileIO::_write_file_check(std::exception& e) noexcept
{
	auto result = false;
	if (!file_)
		e = std::runtime_error("invaild file stream");
	else if ((file_openmode_ & std::ios::out) == false)
		e = std::runtime_error("file stream is not output mode");
	else if ((file_openmode_ & std::ios::binary) == false)
		e = std::runtime_error("file stream is not binary mode");
	else
		result = true;
	return result;
}
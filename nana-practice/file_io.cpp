#include "file_io.hpp"

#include <array>
#include <vector>

namespace file_io
{
	bool IOTextFile::open(std::ios::openmode mode) // needs std::ios::binary
	{
		if (filename_.empty() || file_.is_open())
			return false;
		file_openmode_ = mode;
		file_.open(filename_, file_openmode_);
		return file_.good();
	}

	LocaleEnum IOTextFile::read_bom(std::exception & e)
	{
		std::array<unsigned char, 3> buf;
		if (!_read_file_check(e))
			return LocaleEnum::fail;
		file_.seekg(0, std::ios::end);
		auto file_size = file_.tellg();
		file_.seekg(0, std::ios::beg);
		if (file_size > 1)
		{
			file_.read(&buf[0], 2);
			if (buf[0] == bom::k_u16_le[0] && buf[1] == bom::k_u16_le[1])
				return LocaleEnum::utf16_le;
			if (file_size > 2)
			{
				file_.read(&buf[2], 1);
				if (buf[0] == bom::k_u8[0] && buf[1] == bom::k_u8[1] && buf[2] == bom::k_u8[2])
					return LocaleEnum::utf8;
			}
		}
		file_.seekg(0, std::ios::beg);
		return LocaleEnum::system;
	}

	bool IOTextFile::update_file_locale_by_read_bom(std::exception & e)
	{
		file_locale_ = read_bom(e);
		if (file_locale_ != LocaleEnum::fail)
			return true;
		file_locale_ = LocaleEnum::system;
		return false;
	}

	std::string IOTextFile::read_all(std::exception & e) noexcept
	{
		std::string buf;
		read_all(buf, 0U, true, e);
		return buf;
	}

	std::u16string IOTextFile::read_all_u16(std::exception & e) noexcept
	{
		std::u16string buf;
		read_all(buf, 0U, true, e);
		return buf;
	}

	bool IOTextFile::_read_file_check(std::exception & e)
	{
		auto result = false;
		if (!file_)
			e = std::exception("invaild file stream");
		else if ((file_openmode_ & std::ios::in) == false)
			e = std::exception("file stream is not input mode");
		else if ((file_openmode_ & std::ios::binary) == false)
			e = std::exception("file stream is not binary mode");
		else
			result = true;
		return result;
	}

	bool IOTextFile::_write_file_check(std::exception & e)
	{
		auto result = false;
		if (!file_)
			e = std::exception("invaild file stream");
		else if ((file_openmode_ & std::ios::out) == false)
			e = std::exception("file stream is not output mode");
		else if ((file_openmode_ & std::ios::binary) == false)
			e = std::exception("file stream is not binary mode");
		else
			result = true;
		return result;
	}
}
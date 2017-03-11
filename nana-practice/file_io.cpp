#include "file_io.hpp"

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
		if (!read_file_check(e))
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

	std::string IOTextFile::read_all(std::exception & e)
	{
		std::string buf;

		try
		{
			std::streamoff bom_length;

			if (!update_file_locale_by_read_bom(e)) // includes read_file_check()
				return buf;
			bom_length = file_.tellg();

			// set locale codecvt
			switch (file_locale_)
			{
			case LocaleEnum::system:
			case LocaleEnum::utf8:
				file_.seekg(0, std::ios::end);
				buf.resize(static_cast<std::size_t>(file_.tellg() - bom_length));
				file_.seekg(bom_length, std::ios::beg);
				file_.read(reinterpret_cast<unsigned char*>(&buf[0]), buf.size());
				file_.close();
				break;
			case LocaleEnum::utf16_le:
				e = std::exception("UTF-16 support is not ready yet");
				return buf;
			}
		}
		catch (std::exception& _e)
		{
			e = _e;
		}

		return buf;
	}

	void IOTextFile::write_all(const char * buf)
	{

	}

	bool IOTextFile::read_file_check(std::exception & e)
	{
		auto result = false;
		if (!file_)
			e = std::exception("invaild file stream");
		else if ((file_openmode_ & std::ios::in) == false)
			e = std::exception("not input mode");
		else if ((file_openmode_ & std::ios::binary) == false)
			e = std::exception("not binary mode");
		else
			result = true;
		return result;
	}
}
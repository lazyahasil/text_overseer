﻿#pragma once

#include <fstream>

namespace file_io
{
	namespace bom // Byte Order Mark
	{
		constexpr unsigned char k_u8[3]{ 0xEF, 0xBB, 0xBF };
		constexpr unsigned char k_u16_le[2]{ 0xFF, 0xFE };
		//constexpr unsigned char bom_u16_be[]{ 0xFE, 0xFF };
	}

	enum class LocaleEnum
	{
		fail,
		system,		// can be treated as "UTF-8 without BOM"
		utf8,		// UTF-8 with BOM
		utf16_le	// UTF-16LE
	};

	class IOTextFile
	{
	public:
		IOTextFile() { }
		IOTextFile(
			const std::wstring& filename,
			LocaleEnum file_locale = LocaleEnum::system
		) : filename_(filename), file_locale_(file_locale) { }
		IOTextFile(
			std::wstring&& filename,
			LocaleEnum file_locale = LocaleEnum::system
		) : filename_(std::move(filename)), file_locale_(file_locale) { }

		bool open(std::ios::openmode mode);
		void close() noexcept { file_.close(); }
		decltype(auto) filename() const noexcept { return filename_; }

		template <class StringT>
		void filename(StringT&& filename) noexcept
		{
			filename_ = std::forward<StringT>(filename);
		}

		LocaleEnum file_locale() const noexcept { return file_locale_; }
		void file_locale(LocaleEnum file_locale) noexcept { file_locale_ = file_locale; }
		LocaleEnum read_bom(std::exception& e); // includes _read_file_check()
		bool update_file_locale_by_read_bom(std::exception& e);  // includes read_bom()

		template <class MutableStringBuffer>
		std::size_t read_all(
			MutableStringBuffer&	buf,
			std::size_t				buf_size,
			bool					is_resizable,
			std::exception&			e
		) noexcept
		{
			auto do_write_16_bit = false;
			if (!update_file_locale_by_read_bom(e)) // includes _read_file_check()
				return 0U;
			try
			{
				if (file_locale_ == LocaleEnum::utf16_le)
				{
					if (sizeof(buf[0]) == 2)
						do_write_16_bit = true;
					else if (sizeof(buf[0]) != 1) // buffer type size != 1, 2
						throw std::exception("a mutable byte or 16-bit sequence buffer is needed for UTF-16LE");
				}
				std::streamoff bom_length = file_.tellg();
				file_.seekg(0, std::ios::end);
				auto size = static_cast<std::size_t>(file_.tellg() - bom_length); // string length
				auto sequence_length = do_write_16_bit ? (size / 2 + size % 2) : size;
				if (buf_size < size)
				{
					if (is_resizable)
						_resize_buf(buf, sequence_length);
					else
						throw std::length_error("data size is larger than the buffer size");
				}
				file_.seekg(bom_length, std::ios::beg);
				file_.read(reinterpret_cast<unsigned char*>(&buf[0]), size);
				return sequence_length;
			}
			catch (std::exception& _e)
			{
				e = _e;
			}
			return 0U;
		}

		std::string read_all(std::exception& e) noexcept;
		std::u16string read_all_u16(std::exception& e) noexcept;

		template <class ConstStringBuffer>
		void write_all(ConstStringBuffer buf, std::size_t length, std::exception& e) noexcept
		{
			if (!_write_file_check(e))
				return;
			try
			{
				file_.seekg(0, std::ios::beg);
				if (file_locale_ == LocaleEnum::utf8)
					file_.write(bom::k_u8, 3);
				else if (file_locale_ == LocaleEnum::utf16_le)
					file_.write(bom::k_u16_le, 2);
				file_.write(reinterpret_cast<const unsigned char*>(&buf[0]), length);
			}
			catch (std::exception& _e)
			{
				e = _e;
			}
		}

	private:
		bool IOTextFile::_read_file_check(std::exception & e);
		bool IOTextFile::_write_file_check(std::exception & e);

		template <class ResizableStringBuffer>
		void _resize_buf(ResizableStringBuffer& buf, std::size_t size)
		{
			buf.resize(size); // throws std::length_error if the size is too big
		}

		std::basic_fstream<unsigned char> file_;
		std::ios::openmode file_openmode_;
		LocaleEnum file_locale_;
		std::wstring filename_;
	};
}
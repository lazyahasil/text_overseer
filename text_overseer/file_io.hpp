#pragma once

#include <fstream>

namespace bom // Byte Order Mark
{
	constexpr unsigned char k_u8[3]{ 0xEF, 0xBB, 0xBF };
	constexpr unsigned char k_u16_le[2]{ 0xFF, 0xFE };
	//constexpr unsigned char bom_u16_be[]{ 0xFE, 0xFF };
}

class FileIO
{
public:
	enum class encoding
	{
		unknown,
		system,		// ANSI(system locale); can be treated as "UTF-8 without BOM"
		utf8,		// UTF-8 with BOM
		utf16_le	// UTF-16LE
	};

	FileIO() = default;
	FileIO(
		const std::wstring& filename,
		encoding file_locale = encoding::system
	) : filename_(filename), file_locale_(file_locale) { }
	FileIO(
		std::wstring&& filename,
		encoding file_locale = encoding::system
	) : filename_(std::move(filename)), file_locale_(file_locale) { }

	bool open(std::ios::openmode mode);
	void close() noexcept { file_.close(); }
	const wchar_t* filename() const noexcept { return filename_.c_str(); }
	const std::wstring& filename_wstring() const noexcept { return filename_; }

	template <class StringT>
	void filename(StringT&& filename) noexcept
	{
		filename_ = std::forward<StringT>(filename);
	}

	encoding locale() const noexcept { return file_locale_; }
	void locale(encoding locale) noexcept;
	encoding read_bom(); // includes _read_file_check()
	bool update_locale_by_read_bom();  // includes read_bom()

	template <class MutableStringBuffer>
	std::size_t read_all(
		MutableStringBuffer&	buf,
		std::size_t				buf_size,
		bool					is_resizable
	)
	{
		auto do_write_16_bit = false;
		if (!update_locale_by_read_bom()) // includes _read_file_check()
			return 0U;
		if (file_locale_ == encoding::utf16_le)
		{
			if (sizeof(buf[0]) == 2)
				do_write_16_bit = true;
			else if (sizeof(buf[0]) != 1) // buffer type size != 1, 2
				throw std::runtime_error("a mutable byte or 16-bit sequence buffer is needed for UTF-16LE");
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
		return 0U;
	}

	std::string read_all();
	std::u16string read_all_u16();

	template <class ConstStringBuffer>
	void write_all(ConstStringBuffer buf, std::size_t length)
	{
		if (!_write_file_check())
			return;
		file_.seekg(0, std::ios::beg);
		if (file_locale_ == encoding::utf8)
			file_.write(bom::k_u8, 3);
		else if (file_locale_ == encoding::utf16_le)
			file_.write(bom::k_u16_le, 2);
		file_.write(reinterpret_cast<const unsigned char*>(&buf[0]), length);
	}

private:
	bool FileIO::_read_file_check();
	bool FileIO::_write_file_check();

	template <class ResizableStringBuffer>
	void _resize_buf(ResizableStringBuffer& buf, std::size_t size)
	{
		buf.resize(size); // throws std::length_error if the size is too big
	}

	std::basic_fstream<unsigned char>	file_;
	std::ios::openmode					file_openmode_;
	encoding							file_locale_{ encoding::system };
	std::wstring						filename_;
};
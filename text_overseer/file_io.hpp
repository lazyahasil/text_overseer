#pragma once

#include "encoding.hpp"

#include <array>
#include <fstream>

namespace text_overseer
{
	namespace file_io
	{
		namespace detail
		{
			// bytes size limit when checking if the file is UTF-8 without BOM
			// (there may be a rare chance to check a big file with ANSI or UTF-8 without BOM)
			constexpr std::size_t k_max_size_check_utf8 = 0x800U;

			namespace bom // Byte Order Mark
			{
				constexpr std::array<unsigned char, 3> k_u8{ 0xEF, 0xBB, 0xBF };
				constexpr std::array<unsigned char, 2> k_u16_le{ 0xFF, 0xFE };
				//constexpr std::array<unsigned char, 2> k_u16_be{ 0xFE, 0xFF };
			}

			namespace newline
			{
				constexpr std::array<unsigned char, 2> k_ascii_cr_lf{ 0x0D, 0x0A };
				constexpr std::array<unsigned char, 4> k_u16le_cr_lf{ 0x0D, 0x00, 0x0A, 0x00 };
				//constexpr std::array<unsigned char, 1> k_ascii_lf{ 0x0A };
				//constexpr std::array<unsigned char, 2> k_u16le_lf{ 0x0A, 0x00 };

				inline const auto& ascii() { return k_ascii_cr_lf; } // returns default ascii newline
				inline const auto& u16le() { return k_u16le_cr_lf; } // returns default u16le newline
			}
		}

		// a class that supports text file reading & writing;
		// it also can handle the system encoding and unicode, and can take care of BOM(Byte Order Mark)
		class FileIO
		{
		public:
			enum class encoding
			{
				unknown,		// neutral or fail => can be automatically translated to ANSI
				system,			// ANSI(system locale)
				utf8,			// UTF-8 with BOM
				utf8_no_bom,	// UTF-8 without BOM => will be treated as ANSI if there are ASCII codes only
				utf16_le		// UTF-16LE
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

			bool open(std::ios::openmode mode); // needs std::ios::binary
			bool is_open() noexcept { return file_.is_open(); }
			void close() noexcept { file_.close(); }
			const wchar_t* filename() const noexcept { return filename_.c_str(); }
			const std::wstring& filename_wstring() const noexcept { return filename_; }

			template <class StringT>
			void filename(StringT&& filename) noexcept { filename_ = std::forward<StringT>(filename); }

			long long stream_size(); // includes _read_file_check()
			encoding locale() const noexcept { return file_locale_; }
			void locale(encoding locale) noexcept;
			encoding read_bom(); // includes _read_file_check()
			bool write_bom(); // includes _write_file_check()
			bool update_locale_by_read_bom();  // includes read_bom()

			template <class MutableStringBuffer>
			std::size_t read_all(
				MutableStringBuffer&	buf,
				std::size_t				buf_size,
				bool					is_resizable
			)
			{
				if (!update_locale_by_read_bom()) // includes _read_file_check()
					return 0U;

				// check the byte size of the buffer type (because of resizing)
				auto do_use_16_bit_buffer = false;
				if (file_locale_ == encoding::utf16_le) // when reading UTF-16LE
				{
					if (sizeof(buf[0]) == 2)
						do_use_16_bit_buffer = true;
					else if (sizeof(buf[0]) != 1) // buffer type size != 1, 2
						throw std::invalid_argument("a mutable byte or 16-bit sequence buffer is needed for UTF-16LE");
				}
				else if (sizeof(buf[0]) != 1) // buffer type size != 1
				{
					throw std::invalid_argument("a mutable byte(8-bit) sequence buffer is needed");
				}

				const auto bom_length = file_.tellg();
				const auto byte_size = static_cast<std::size_t>(stream_size() - bom_length);
				const auto sequence_length = do_use_16_bit_buffer ? (byte_size / 2 + byte_size % 2) : byte_size;

				// resize the buffer
				if (buf_size < byte_size)
				{
					if (is_resizable)
						_resize_buf(buf, sequence_length);
					else
						throw std::length_error("data size is larger than the buffer size");
				}

				file_.seekg(bom_length, std::ios::beg);
				file_.read(reinterpret_cast<unsigned char*>(&buf[0]), byte_size);

				// when encoding is system, check if it's UTF-8 without BOM, within certain bytes of string
				if (file_locale_ == encoding::system)
				{
					if (utf8_check_vaild(buf, k_max_size_check_utf8, true))
						file_locale_ = encoding::utf8_no_bom;
				}

				return sequence_length;
			}

			std::string read_all();
			std::u16string read_all_u16();

			template <class StringBuffer>
			bool write_all(const StringBuffer& buf, std::size_t byte_length)
			{
				if (!write_bom()) // includes _write_file_check()
					return false;
				file_.write(reinterpret_cast<const unsigned char*>(&buf[0]), byte_length);
				return true;
			}

			template <class StringBuffer>
			bool write_some(const StringBuffer& buf, std::size_t byte_length)
			{
				if (file_.tellg() == 0LL)
				{
					// write BOM if the file is empty
					if (!write_bom()) // includes _write_file_check()
						return false;
				}
				else
				{
					if (!_write_file_check())
						return false;
				}
				file_.write(reinterpret_cast<const unsigned char*>(&buf[0]), byte_length);
				return true;
			}

			template <class ConstStringBuffer1, class ConstStringBuffer2>
			bool write_line(
				const ConstStringBuffer1& buf,
				std::size_t byte_length,
				const ConstStringBuffer2& newline
			)
			{
				// perform write_some()
				if (!write_some(buf, byte_length))
					return false;
				// write a newline
				file_.write(reinterpret_cast<const unsigned char*>(&newline[0]), newline.size());
				return true;
			}

			template <class ConstStringBuffer>
			bool write_line(const ConstStringBuffer& buf, std::size_t byte_length)
			{
				if (file_locale_ == encoding::utf16_le)
					return write_line(buf, byte_length, detail::newline::u16le());
				return write_line(buf, byte_length, detail::newline::ascii());
			}

		protected:
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

		// a simple guard class for FileIO; it automatically closes the file
		class FileIOClosingGuard
		{
		public:
			explicit FileIOClosingGuard() = delete;
			explicit FileIOClosingGuard(FileIO& file_io) : file_io_(&file_io) { }

			~FileIOClosingGuard() { close_safe(); }

			FileIOClosingGuard(const FileIOClosingGuard& src) = delete;
			FileIOClosingGuard& operator=(const FileIOClosingGuard& rhs) = delete;

			void close_safe() noexcept
			{
				// no need for check because std::basic_fstream<>::close() does check if closed
				file_io_->close();
			}

		private:
			FileIO* file_io_{ nullptr };
			bool is_closed_{ false };
		};
	}
}
#pragma once

#include <array>
#include <codecvt>
#include <fstream>

namespace file_io
{
	namespace bom
	{
		constexpr unsigned char k_u8[3]{ 0xEF, 0xBB, 0xBF };
		constexpr unsigned char k_u16_le[2]{ 0xFF, 0xFE };
		//constexpr char bom_u16_be[]{ 0xFE, 0xFF };
	}

	enum class LocaleEnum
	{
		fail,
		system,		// system can be treated as "UTF-8 without BOM"
		utf8,		// UTF-8 with BOM
		utf16_le
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
		void set_filename(StringT&& filename) noexcept
		{
			filename_ = std::forward<StringT>(filename);
		}

		LocaleEnum file_locale() const noexcept { return file_locale_; }
		void set_file_locale(LocaleEnum file_locale) noexcept { file_locale_ = file_locale; }
		LocaleEnum read_bom(std::exception& e);
		bool update_file_locale_by_read_bom(std::exception& e);

		std::string read_all(std::exception& e);
		void write_all(const char* buf);

	private:
		bool read_file_check(std::exception& e);

		std::basic_fstream<unsigned char> file_;
		std::ios::openmode file_openmode_;
		LocaleEnum file_locale_;
		std::wstring filename_;
	};
}
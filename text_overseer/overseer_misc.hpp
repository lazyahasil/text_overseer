#pragma once

#include <codecvt>

namespace overseer_misc
{
	constexpr char* k_version_str = "0.2.5.1";

	// the functions of namespace encoding_convert are needed for certain reasons below
	// 1. nana::charset() doesn't fully support wide -> ANSI conversion
	// 2. nana::charset() doesn't fully support unicode -> unicode conversion
	//    (for example, wide -> UTF-8, UTF-8 -> UTF-16)
	inline namespace encoding_convert
	{
		// custom encoding facet structure for wstr_to_mstr()
		template <class Facet>
		struct DeletableFacet : Facet
		{
			using Facet::Facet; // inherit constructors
			~DeletableFacet() = default;
		};

		// @throws std::range_error: bad conversion caused by some unicode characters
		template <class ConstWstringContainer>
		std::string wstr_to_mstr(const ConstWstringContainer& wstr)
		{
			using LocalFacet = DeletableFacet<std::codecvt_byname<wchar_t, char, std::mbstate_t>>;
			std::wstring_convert<LocalFacet> converter(new LocalFacet(""));
			return converter.to_bytes(wstr);
		}

		template <class ConstWstringContainer>
		std::string wstr_to_utf8(const ConstWstringContainer& wstr)
		{
			std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
			return converter.to_bytes(wstr);
		}

		template <class ConstStringContainer>
		std::u16string utf8_to_utf16(const ConstStringContainer& u8_str)
		{
#if _MSC_VER == 1900 /*VS 2015*/ || _MSC_VER == 1910 /*VS 2017*/
			// Visual Studio bug: https://connect.microsoft.com/VisualStudio/feedback/details/1403302
			std::wstring_convert<std::codecvt_utf8_utf16<int16_t>, int16_t> converter;
			auto u16_int_str = converter.from_bytes(u8_str);
			std::u16string u16_buf;
			u16_buf.assign(reinterpret_cast<const char16_t*>(u16_int_str.data()), u16_int_str.size());
			return u16_buf;
#else
			std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
			return converter.from_bytes(u8_str);
#endif
		}
	}

	// source: http://www.zedwood.com/article/cpp-is-valid-utf8-string-function
	template <class ConstStringContainer>
	bool utf8_check_vaild(const ConstStringContainer& str, std::size_t length_limit)
	{
		std::size_t len;
		if (length_limit == std::string::npos)
			len = str.length();
		else
			len = std::min(length_limit, str.length());
		for (std::size_t i = 0; i < len; i++)
		{
			int c = static_cast<unsigned char>(str[i]);
			std::size_t n;
			//if (c==0x09 || c==0x0a || c==0x0d || (c >= 0x20 && c <= 0x7e) ) n = 0; // is_printable_ascii
			if (c >= 0x00 && c <= 0x7f) // 0bbbbbbb
				n = 0;
			else if ((c & 0xE0) == 0xC0) // 110bbbbb
				n = 1;
			else if ( c == 0xED && i < len - 1
				&& (static_cast<unsigned char>(str[i + 1]) & 0xA0) == 0xA0 ) //U+d800 to U+dfff
				return false;
			else if ((c & 0xF0) == 0xE0) // 1110bbbb
				n = 2;
			else if ((c & 0xF8) == 0xF0) // 11110bbb
				n = 3;
			//else if ((c & 0xFC) == 0xF8) n=4; // 111110bb // byte 5, unnecessary in 4 byte UTF-8
			//else if ((c & 0xFE) == 0xFC) n=5; // 1111110b // byte 6, unnecessary in 4 byte UTF-8
			else
				return false;
			for (std::size_t j = 0; j < n && i < len; j++) // n bytes matching 10bbbbbb follow ?
			{
				if ( ( ++i == len && length_limit == std::string::npos )
					|| ( (static_cast<unsigned char>(str[i]) & 0xC0) != 0x80 ) )
					return false;
			}
		}
		return true;
	}

	template <class ConstStringContainer>
	bool utf8_check_vaild(const ConstStringContainer& str)
	{
		return utf8_check_vaild(str, std::string::npos);
	}
}
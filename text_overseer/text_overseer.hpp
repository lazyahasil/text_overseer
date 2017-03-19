#pragma once

#include <codecvt>

namespace text_overseer
{
	constexpr char* k_version_str = "0.2.4.0";

	template <class Facet>
	struct DeletableFacet : Facet
	{
		using Facet::Facet; // inherit constructors
		~DeletableFacet() = default;
	};

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
	std::u16string utf16_to_utf8(const ConstStringContainer& u8_str)
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
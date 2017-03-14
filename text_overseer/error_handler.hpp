#pragma once

#include "file_io.hpp"
#include "singleton.hpp"

class ErrorHandler;
using ErrorHdr = ErrorHandler;

class ErrorHandler final : public Singleton<ErrorHandler>
{
public:
	enum class interface
	{
		ignore,
		file_log,
		gui_msgbox
	};

	enum class priority
	{
		info,
		warning,
		critical
	};

	ErrorHandler() = default;

	void report(priority p, int error_code, const char* u8_str)
	{

	}

	void report(priority p, int error_code, const char* u8_str, const char* postfix_u8_str)
	{

	}

	void report(priority p, int error_code, const char* u8_str, const wchar_t* postfix_wstr)
	{

	}

private:
	
};
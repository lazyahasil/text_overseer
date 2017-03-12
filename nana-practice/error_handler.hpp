#pragma once

#include "file_io.hpp"
#include "singleton.hpp"

class ErrorHandler final : public Singleton<ErrorHandler>
{
public:
	enum class Interface
	{
		ignore,
		file_log,
		gui_msgbox
	};

	enum class Priority
	{
		info,
		warning,
		critical
	};

	ErrorHandler() = default;

	void report(Priority priority, int error_code, const char* u8_str)
	{

	}

	void report(Priority priority, int error_code, const char* u8_str, const char* postfix_u8_str)
	{

	}

	void report(Priority priority, int error_code, const char* u8_str, const wchar_t* postfix_wstr)
	{

	}

private:
	
};
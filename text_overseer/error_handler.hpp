#pragma once

#include "text_overseer.hpp"
#include "file_io.hpp"
#include "file_system.hpp"
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
	
	void start() { started = true; }
	bool is_started() const { return started; }

	template <class ConstStringContainer>
	void report(priority p, int error_code, const ConstStringContainer& u8_str)
	{
		if (!started)
			return;
		try
		{
			file_.open(std::ios::app | std::ios::binary);
			std::ostringstream stream;
			stream << "[" << priority_str(p) << "] " << u8_str << " (" << error_code << ") ";
			std::string msg = stream.str();
			file_.write_line(msg, msg.size());
			file_.close();
		}
		catch (std::exception&)
		{
			// do nothing
		}
	}

	template <class ConstStringContainer1, class ConstStringContainer2>
	void report(
		priority p,
		int error_code,
		const ConstStringContainer1& u8_str,
		const ConstStringContainer2& postfix_u8_str
	)
	{
		if (!started)
			return;
		try
		{
			file_.open(std::ios::app | std::ios::binary);
			std::ostringstream stream;
			stream << "[" << priority_str(p) << "] " << u8_str << " (" << error_code << "): " << postfix_u8_str;
			std::string msg = stream.str();
			file_.write_line(msg, msg.size());
			file_.close();
		}
		catch (std::exception&)
		{
			// do nothing
		}
	}

private:
	const char* priority_str(priority p)
	{
		if (p == priority::info)
			return "iInformation";
		if (p == priority::warning)
			return "Warning";
		if (p == priority::critical)
			return "Critical";
		return "";
	}

	bool started{ false };
	FileIO file_{ L"text_overseer.log", FileIO::encoding::utf8 };
};

#define ErrorHandler // don't use ErrorHandler; use ErrorHdr
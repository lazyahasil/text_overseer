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
	
	void start()
	{
		file_.open(std::ios::app | std::ios::binary);
		started = true;
	}

	bool is_started() const { return started; }

	template <class ConstStringContainer>
	void report(priority p, int error_code, const ConstStringContainer& u8_str)
	{
		if (!started)
			return;
		try
		{
			std::ostringstream stream;
			stream << "[" << priority_str(p) << "] " << u8_str << " (" << error_code << ") ";
			std::string msg = stream.str();
			file_.write_line(msg, msg.size());
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
			std::ostringstream stream;
			stream << "[" << priority_str(p) << "] " << u8_str << " (" << error_code << "): " << postfix_u8_str;
			std::string msg = stream.str();
			file_.write_line(msg, msg.size());
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
			return "information";
		if (p == priority::warning)
			return "warning";
		if (p == priority::critical)
			return "critical";
		return "";
	}

	bool started{ false };
	FileIO file_{ L"text_overseer.log", FileIO::encoding::utf8 };
};
#pragma once

#include "file_io.hpp"
#include "singleton.hpp"

namespace error_handler
{
	class ErrorHdr final : public Singleton<ErrorHdr>
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

		ErrorHdr() = default;

		void start() noexcept { started = true; }
		bool is_started() const noexcept { return started; }

		template <class ConstStringContainer>
		void report(priority p, int error_code, const ConstStringContainer& u8_str) noexcept
		{
			if (!started)
				return;
			try
			{
				FileIOClosingGuard file_closer(file_);
				file_.open(std::ios::app | std::ios::binary);
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
		) noexcept
		{
			if (!started)
				return;
			try
			{
				FileIOClosingGuard file_closer(file_);
				file_.open(std::ios::app | std::ios::binary);
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
		const char* priority_str(priority p) noexcept
		{
			if (p == priority::info)
				return "Information";
			if (p == priority::warning)
				return "Warning";
			if (p == priority::critical)
				return "Critical";
			return "";
		}

		bool started{ false };
		file_io::FileIO file_{ L"text_overseer.log", file_io::FileIO::encoding::utf8 };
	};
}
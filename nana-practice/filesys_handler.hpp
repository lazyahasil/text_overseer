#pragma once

#include <boost/system/error_code.hpp>
#include <boost/lexical_cast.hpp>
#include <chrono>
#include <string>
#include <utility> // std::pair
#include <vector>

namespace filesys_handler
{
	class FilePathErrorCode
	{
	public:
		FilePathErrorCode() = default;
		FilePathErrorCode(const std::wstring& path_str, boost::system::error_code ec)
			: path_str_(path_str), ec_(ec) { }
		FilePathErrorCode(std::wstring&& path_str, boost::system::error_code ec)
			: path_str_(move(path_str)), ec_(ec) { }

		auto path_str() const noexcept
		{
			return path_str_;
		}
		auto error_code() const noexcept
		{
			return ec_;
		}

	private:
		std::wstring path_str_;
		boost::system::error_code ec_;
	};

	// @param filename: file name to search files match it
	// @param dir_path: directory path to start a search
	// @param initialized_buf: template container with std::wstring;
	//        it should have push_back(), and shall be empty before call this func
	// @param ecs_with_path: buffer for error codes with files' path string
	// @param do_search_subfolders: boolean switch whether to search subfolders or not;
	//        a default value is false
	template <class MutableContainerForWstring>
	void search_files(
		const std::wstring&				filename,
		std::wstring					dir_path,
		MutableContainerForWstring&		initialized_buf,
		std::vector<FilePathErrorCode>& ecs_with_path,
		bool							do_search_subfolders
	) noexcept
	{
		boost::system::error_code ec;

		if (!filesys::is_directory(dir_path, ec))
		{
			ecs_with_path.emplace_back(dir_path, ec);
			return;
		}

		for (const filesys::directory_entry& x : filesys::directory_iterator(dir_path, ec))
		{
			ec.clear();
			if (filesys::is_regular_file(x.path(), ec) && x.path().filename() == filename)
				initialized_buf.push_back(x.path().wstring());
			else if (do_search_subfolders && filesys::is_directory(x.path(), ec))
				// search on the subfolders
				search_files(filename, x.path().wstring(), initialized_buf, ecs_with_path, true);
			if (ec)
			{
				ecs_with_path.emplace_back(x.path().wstring(), ec);
			}
		}
	}

	using IOFilePathPair = std::pair<std::wstring, std::wstring>;

	// @param input_filename: file name of input file
	// @param output_filename: file name of output file
	// @param dir_path: directory path to start a search;
	//        a default value is the current directory path, where locates this program
	std::pair<std::vector<IOFilePathPair>, std::vector<FilePathErrorCode>>
		search_input_output_files(
			const std::wstring& input_filename,
			const std::wstring& output_filename,
			const std::wstring& dir_path
		) noexcept;

	// @param file_path: 
	std::chrono::time_point<std::chrono::system_clock> file_last_write_time(
		const std::wstring&			file_path,
		boost::system::error_code&	ec
	) noexcept;

	template <class StringType>
	struct TimePeriodStrings
	{
		StringType just;
		StringType msecs_singular;
		StringType msecs_plural;
		StringType secs_singular;
		StringType secs_plural;
		StringType mins_singular;
		StringType mins_plural;
		StringType hours_singular;
		StringType hours_plural;
		StringType days_singular;
		StringType days_plural;
	};

	const TimePeriodStrings<std::string> k_time_period_strings_default{
		"just a few", " millisecond", " milliseconds", " second", " seconds",
		" minute", " minutes", " hour", " hours", " day", " days"
	};

	template <class StringType, class Rep, class Period>
	StringType time_duration_to_string(
		const	std::chrono::duration<Rep, Period>& duration,
		bool	do_cut_smaller_periods,
		const	TimePeriodStrings<StringType>& periods
	)
	{
		enum class PeriodEnum { msecs, secs, mins, hours, days } base_period;

		auto counted = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

		auto do_return_just = false;
		if (counted == 0)
			do_return_just = true;
		
		if (std::ratio_less_equal<Period, std::milli>())
			base_period = PeriodEnum::msecs;
		else if (std::ratio_less_equal<Period, std::ratio<1>>())
			base_period = PeriodEnum::secs;
		else if (std::ratio_less_equal<Period, std::ratio<60>>())
			base_period = PeriodEnum::mins;
		else if (std::ratio_less_equal<Period, std::ratio<3600>>())
			base_period = PeriodEnum::hours;
		else
			base_period = PeriodEnum::days;

		const auto msecs = counted % 1000;
		counted /= 1000;
		const auto secs = counted % 60;
		counted /= 60;
		const auto mins = counted % 60;
		counted /= 60;
		const auto hours = counted % 24;
		counted /= 24;
		const auto days = counted;

		// check if the duration is smaller than a base period
		const StringType* period = nullptr;
		if ((do_return_just || msecs == 0) && base_period == PeriodEnum::msecs)
		{
			period = &periods.msecs_singular;
			do_return_just = true;
		}
		else if ((do_return_just || secs == 0) && base_period == PeriodEnum::secs)
		{
			period = &periods.secs_singular;
			do_return_just = true;
		}
		else if ((do_return_just || mins == 0) && base_period == PeriodEnum::mins)
		{
			period = &periods.mins_singular;
			do_return_just = true;
		}
		else if ((do_return_just || hours == 0) && base_period == PeriodEnum::hours)
		{
			period = &periods.hours_singular;
			do_return_just = true;
		}
		else if ((do_return_just || days == 0) && base_period == PeriodEnum::days)
		{
			period = &periods.days_singular;
			do_return_just = true;
		}

		StringType str;

		if (do_return_just) // return just a moment
		{
			str = periods.just + *period;
			return str;
		}

		auto did_eariler = false;

		if (days >= 1)
		{
			str = boost::lexical_cast<StringType>(days)
				+ (days != 1 ? periods.days_plural : periods.days_singular);
			did_eariler = true;
		}
		if ((!do_cut_smaller_periods || !did_eariler) && base_period <= PeriodEnum::hours && hours >= 1)
		{
			str += boost::lexical_cast<StringType>(hours)
				+ (hours != 1 ? periods.hours_plural : periods.hours_singular);
			did_eariler = true;
		}
		if ((!do_cut_smaller_periods || !did_eariler) && base_period <= PeriodEnum::mins && mins >= 1)
		{
			str += boost::lexical_cast<StringType>(mins)
				+ (mins != 1 ? periods.mins_plural : periods.mins_singular);
			did_eariler = true;
		}
		if ((!do_cut_smaller_periods || !did_eariler) && base_period <= PeriodEnum::secs && secs >= 1)
		{
			str += boost::lexical_cast<StringType>(secs)
				+ (secs != 1 ? periods.secs_plural : periods.secs_singular);
			did_eariler = true;
		}
		if ((!do_cut_smaller_periods || !did_eariler) && base_period <= PeriodEnum::msecs && msecs >= 1)
		{
			str += boost::lexical_cast<StringType>(msecs)
				+ (msecs != 1 ? periods.msecs_plural : periods.msecs_singular);
			did_eariler = true;
		}

		return str;
	}
}
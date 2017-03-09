#pragma once

#include <boost/system/error_code.hpp>
#include <boost/filesystem.hpp>
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
	// @param initialized_buf: empty(initialized) mutable container consist of std::wstring;
	//        it is to be used by .insert(), and shall be empty before call this func
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
				initialized_buf.insert(std::end(initialized_buf), x.path().wstring());
			else if (do_search_subfolders && filesys::is_directory(x.path(), ec))
				// search on the subfolders
				search_files(filename, x.path().wstring(), initialized_buf, ecs_with_path, true);
			if (ec)
				ecs_with_path.emplace_back(x.path().wstring(), ec);
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

	template <class StringT>
	struct TimePeriodStrings
	{
		StringT just;
		StringT msecs_singular;
		StringT msecs_plural;
		StringT secs_singular;
		StringT secs_plural;
		StringT mins_singular;
		StringT mins_plural;
		StringT hours_singular;
		StringT hours_plural;
		StringT days_singular;
		StringT days_plural;
	};

	namespace time_period_strings
	{
		const TimePeriodStrings<char*> k_english{
			"just a few", " ms ", " ms ", " second ", " seconds ",
			" minute ", " minutes ", " hour ", " hours ", " day ", " days "
		};
		const TimePeriodStrings<char*> k_korean_u8{
			u8"몇", " ms ", " ms ", u8" 초 ", u8" 초 ",
			u8" 분 ", u8" 분 ", u8" 시간 ", u8" 시간 ", u8" 일 ", u8" 일 "
		};
	}

	template <class StringT, class PeriodStringT, class Rep, class Period>
	StringT time_duration_to_string(
		const std::chrono::duration<Rep, Period>&	duration,
		bool										do_cut_smaller_periods,
		const TimePeriodStrings<PeriodStringT>&		periods
	)
	{
		enum class PeriodEnum { msecs, secs, mins, hours, days } base_period;
		auto counted = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

		auto do_return_just = false;
		if (counted == 0)
			do_return_just = true;

		if (std::ratio_greater_equal<Period, std::ratio<86400>>())
			base_period = PeriodEnum::days;
		else if (std::ratio_greater_equal<Period, std::ratio<3600>>())
			base_period = PeriodEnum::hours;
		else if (std::ratio_greater_equal<Period, std::ratio<60>>())
			base_period = PeriodEnum::mins;
		else if (std::ratio_greater_equal<Period, std::ratio<1>>())
			base_period = PeriodEnum::secs;
		else
			base_period = PeriodEnum::msecs;

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
		const PeriodStringT* period = nullptr;
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

		StringT str;

		if (do_return_just) // return just a moment
		{
			str = periods.just + *period;
			return str;
		}

		auto did_eariler = false;

		if (days >= 1)
		{
			str = boost::lexical_cast<StringT>(days)
				+ (days != 1 ? periods.days_plural : periods.days_singular);
			did_eariler = true;
		}
		if ((!do_cut_smaller_periods || !did_eariler) && base_period <= PeriodEnum::days && hours >= 1)
		{
			str += boost::lexical_cast<StringT>(hours)
				+ (hours != 1 ? periods.hours_plural : periods.hours_singular);
			did_eariler = true;
		}
		if ((!do_cut_smaller_periods || !did_eariler) && base_period <= PeriodEnum::hours && mins >= 1)
		{
			str += boost::lexical_cast<StringT>(mins)
				+ (mins != 1 ? periods.mins_plural : periods.mins_singular);
			did_eariler = true;
		}
		if ((!do_cut_smaller_periods || !did_eariler) && base_period <= PeriodEnum::mins && secs >= 1)
		{
			str += boost::lexical_cast<StringT>(secs)
				+ (secs != 1 ? periods.secs_plural : periods.secs_singular);
			did_eariler = true;
		}
		if ((!do_cut_smaller_periods || !did_eariler) && base_period <= PeriodEnum::secs && msecs >= 1)
		{
			str += boost::lexical_cast<StringT>(msecs)
				+ (msecs != 1 ? periods.msecs_plural : periods.msecs_singular);
			did_eariler = true;
		}

		return str;
	}

	template <class StringT, class Rep, class Period>
	StringT time_duration_to_string(
		const std::chrono::duration<Rep, Period>&	duration,
		bool										do_cut_smaller_periods,
		const TimePeriodStrings<StringT>&			periods
	)
	{
		return time_duration_to_string<StringT>(duration, do_cut_smaller_periods, periods);
	}

	// a function template of time_duration_to_string for the pointer character types
	// it is called if TimePeriodStrings equals CharT* and StringT equals std::basic_string<CharT>.
	template <class StringT, class CharT, class Rep, class Period,
		typename std::enable_if_t<
		std::is_same< StringT, std::basic_string<CharT> >::value >
	>
	StringT time_duration_to_string(
		const std::chrono::duration<Rep, Period>&	duration,
		bool										do_cut_smaller_periods,
		const TimePeriodStrings<CharT*>&			periods
	)
	{
		return time_duration_to_string<StringT>(duration, do_cut_smaller_periods, periods);
	}
}
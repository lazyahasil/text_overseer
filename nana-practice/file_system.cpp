#include "file_system.hpp"

#include <boost/filesystem.hpp>

namespace file_system
{
	TimePointOfSys file_last_write_time(
		const std::wstring&			file_path,
		boost::system::error_code&	ec
	) noexcept
	{
		std::time_t time = filesys::last_write_time(filesys::path(file_path), ec);
		return std::chrono::system_clock::from_time_t(time); // from_time_t(): noexcept
	}

	std::pair<std::vector<IOFilePathPair>, std::vector<FilePathErrorCode>>
		search_input_output_files(
			const std::wstring& input_filename,
			const std::wstring& output_filename,
			bool				do_always_create_output_path = true,
			const std::wstring& dir_path = filesys::current_path().wstring()
		) noexcept
	{
		std::vector<IOFilePathPair> io_files;
		std::vector<FilePathErrorCode> ecs;
		std::vector<std::wstring> input_files;

		// search for the input files
		search_files(input_filename, dir_path, input_files, ecs, true);

		if (do_always_create_output_path)
		{
			for (const auto& input_file : input_files)
			{
				auto output_file = input_file.substr(0, input_file.find_last_of(L'/'));
				output_file += output_filename;
				io_files.push_back(std::make_pair(input_file, std::move(output_file)));
			}
		}
		else
		{
			// search for the output files
			for (const auto& input_file : input_files)
			{
				auto output_file = input_file.substr(0, input_file.find_last_of(L'/'));
				output_file += output_filename;

				boost::system::error_code ec;
				if (filesys::is_regular_file(filesys::path(output_file), ec))
					io_files.push_back(std::make_pair(input_file, std::move(output_file)));
				else
					io_files.push_back(std::make_pair(input_file, std::wstring()));
			}
		}

		return std::make_pair(io_files, ecs);
	}
}
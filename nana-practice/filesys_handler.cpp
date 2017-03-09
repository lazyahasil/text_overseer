#include "filesys_handler.hpp"

#include <boost/filesystem.hpp>

namespace filesys = boost::filesystem;

namespace filesys_handler
{
	std::pair<std::vector<IOFilePathPair>, std::vector<FilePathErrorCode>>
		search_input_output_files(
			const std::wstring& input_filename,
			const std::wstring& output_filename,
			const std::wstring& dir_path = filesys::current_path().wstring()
		) noexcept
	{
		std::vector<IOFilePathPair> io_files;
		std::vector<FilePathErrorCode> ecs;
		std::vector<std::wstring> input_files;

		// search for the input files
		search_files(input_filename, dir_path, input_files, ecs, true);

		// search for the output files
		for (const auto& input_file : input_files)
		{
			std::vector<std::wstring> output_file_vec;
			search_files(
				output_filename,
				filesys::path(input_file).parent_path().wstring(), output_file_vec, ecs, false
			);

			if (!output_file_vec.empty())
				io_files.push_back(std::make_pair(input_file, output_file_vec[0]));
			else
				io_files.push_back(std::make_pair(input_file, std::wstring()));
		}

		return std::make_pair(io_files, ecs);
	}

	std::chrono::time_point<std::chrono::system_clock> file_last_write_time(
		const std::wstring&			file_path,
		boost::system::error_code&	ec
	) noexcept
	{
		std::time_t time = filesys::last_write_time(filesys::path(file_path), ec);
		return std::chrono::system_clock::from_time_t(time); // from_time_t(): noexcept
	}
}
#include "file_system.hpp"

namespace file_system
{
	TimePointOfSys file_last_write_time(
		const std::wstring&			file_path,
		boost::system::error_code&	ec
	) noexcept
	{
		std::time_t time = filesys::last_write_time(filesys::path(file_path), ec);
		if (ec)
			return TimePointOfSys();
		return std::chrono::system_clock::from_time_t(time); // from_time_t(): noexcept
	}

	std::pair<std::vector<IOFilePathPair>, std::vector<FilePathErrorCode>>
		search_input_output_files(
			const std::wstring& input_filename,
			const std::wstring& output_filename,
			bool				do_always_create_both_path,
			const std::wstring& dir_path
		) noexcept
	{
		std::vector<IOFilePathPair> io_files;
		std::vector<FilePathErrorCode> ecs_with_path;

		search_file_pairs(std::make_pair(input_filename, output_filename), dir_path, io_files, ecs_with_path, true);

		if (do_always_create_both_path)
		{
			for (auto& file_pair : io_files)
			{
				if (file_pair.second.empty())
				{
					file_pair.second = file_pair.first.substr(0, file_pair.first.find_last_of(L"/\\") + 1)
						+ output_filename;
				}
				else if (file_pair.first.empty())
				{
					file_pair.first = file_pair.second.substr(0, file_pair.second.find_last_of(L"/\\") + 1)
						+ input_filename;
				}
			}
		}

		return std::make_pair(io_files, ecs_with_path);
	}
}
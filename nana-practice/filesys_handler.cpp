#include "filesys_handler.hpp"

#include <boost/filesystem.hpp>

namespace filesys = boost::filesystem;

namespace filesys_handler
{
	template <class MutableContainerForWstring>
	void search_files(
		const std::wstring&				filename,
		std::wstring					dir_path,
		MutableContainerForWstring&		initialized_buf,
		std::vector<FilePathErrorCode>& ecs_with_path,
		bool							do_search_subfolders = false
	) noexcept
	{
		boost::system::error_code ec;

		if (!filesys::is_directory(dir_path, ec))
		{
			ecs.emplace_back(dir_path, ec);
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
				ecs_with_path.emplace_back(x.path().wstring(), ec);
		}
	}

	std::pair<std::vector<IOFilePathStr>, std::vector<FilePathErrorCode>>
		search_input_output_files(
			const std::wstring& input_filename,
			const std::wstring& output_filename,
			const std::wstring& dir_path = filesys::current_path().wstring()
		) noexcept
	{
		std::vector<IOFilePathStr> io_files;
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
				io_files.emplace_back(input_file, output_file_vec[0]);
			else
				io_files.emplace_back(input_file, std::wstring());
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
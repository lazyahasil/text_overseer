#include "FileHandler.hpp"

#include <boost/filesystem.hpp>

namespace filesys = boost::filesystem;

namespace FileHandler
{
	template <class MutableContainerForWstring>
	void search_files(
		const std::wstring& filename,
		std::wstring dir_path,
		MutableContainerForWstring& initialized_buf,
		bool do_search_subfolders = false)
	{
		try
		{
			if (!filesys::is_directory(dir_path))
				return;

			for (const filesys::directory_entry& x : filesys::directory_iterator(dir_path))
			{
				if (filesys::is_regular_file(x.path()) && x.path().filename() == filename)
					initialized_buf.push_back(x.path().wstring());
				else if (do_search_subfolders && filesys::is_directory(x.path()))
					search_files(filename, x.path().wstring(), initialized_buf); // search on the subfolders
			}
		}
		catch (const filesys::filesystem_error& e)
		{
			assert(e.what());
		}
	}

	std::vector<std::pair<std::wstring, std::wstring>>
		search_input_output_files(
		const std::wstring& input_filename = L"input.txt",
		const std::wstring& output_filename = L"output.txt",
		const std::wstring& dir_path = filesys::current_path().wstring())
	{
		std::vector<std::pair<std::wstring, std::wstring>> file_pairs;
		std::vector<std::wstring> input_files;

		// search for the input files
		search_files(input_filename, dir_path, input_files, true);

		// search for the output files
		for (const auto& input_file : input_files)
		{
			std::vector<std::wstring> output_file_vec;
			search_files(
				output_filename,
				filesys::path(input_file).parent_path().wstring(), output_file_vec, false
			);
			if (!output_file_vec.empty())
				file_pairs.push_back(std::make_pair(input_file, output_file_vec[0]));
			else
				file_pairs.push_back(std::make_pair(input_file, std::wstring()));
		}

		return file_pairs;
	}
}
#include "ProjDetector.hpp"

#include <boost/filesystem.hpp>

namespace filesys = boost::filesystem;

namespace ProjDetector
{
	std::vector<std::pair<std::wstring, std::wstring>> ProjDetector::search_input_output_files(
		const std::wstring& input_filename,
		const std::wstring& output_filename,
		const std::wstring& dir_path = filesys::current_path().wstring())
	{
		std::vector<std::pair<std::wstring, std::wstring>> file_pairs;
		std::vector<std::wstring> input_files;

		// search the input files
		search_file(input_filename, dir_path, input_files, true);

		// search the output files
		for (auto& input_file : input_files)
		{
			std::vector<std::wstring> output_file_vec;
			search_file(
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

	template <class MutableContainerForWstring>
	void search_file(
		const std::wstring& filename,
		std::wstring dir_path,
		MutableContainerForWstring& initialized_buf,
		bool do_search_subfolders = false)
	{
		filesys::path current_dir(dir_path);

		try
		{
			if (!filesys::is_directory(current_dir))
				return;

			for (filesys::directory_entry& x : filesys::directory_iterator(current_dir))
			{
				if (filesys::is_regular_file(x.path()) && x.path().filename() == filename)
					initialized_buf.push_back(x.path().wstring());
				else if (do_search_subfolders && filesys::is_directory(x.path()))
					search_file(filename, x.path().wstring(), initialized_buf); // search on subfolders
			}
		}
		catch (const filesys::filesystem_error& e)
		{
			assert(e.what());
		}
	}
}
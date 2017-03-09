#pragma once

#include <boost/system/error_code.hpp>
#include <chrono>
#include <string>
#include <utility> // std::pair
#include <vector>

namespace filesys_handler
{
	struct FilePathErrorCode
	{
		std::wstring path_str;
		boost::system::error_code ec;
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
		bool							do_search_subfolders = false
	) noexcept;

	struct IOFilePathStr
	{
		std::wstring input_path;
		std::wstring output_path;
	};

	// @param input_filename: file name of input file
	// @param output_filename: file name of output file
	// @param dir_path: directory path to start a search;
	//        a default value is the current directory path, where locates this program
	std::pair<std::vector<IOFilePathStr>, std::vector<FilePathErrorCode>>
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


}
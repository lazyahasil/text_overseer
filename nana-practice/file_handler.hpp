#pragma once

#include <string>
#include <utility> // std::pair
#include <vector>

namespace file_handler
{
	// @param filename: file name to search files match it
	// @param dir_path: directory path to start a search
	// @param initialized_buf: template container with std::wstring;
	//        it should have push_back(), and shall be empty before call this func
	// @param do_search_subfolders: boolean switch whether to search subfolders or not;
	//        a default value is false
	template <class MutableContainerForWstring>
	void search_files(
		const std::wstring& filename,
		std::wstring dir_path,
		MutableContainerForWstring& initialized_buf,
		bool do_search_subfolders
	);

	// @param input_filename: a default value is L"input.txt"
	// @param output_filename: a default value is L"output.txt"
	// @param dir_path: directory path to start a search;
	//        a default value is the current directory path, where locates this program
	std::vector<std::pair<std::wstring, std::wstring>> search_input_output_files(
		const std::wstring& input_filename,
		const std::wstring& output_filename,
		const std::wstring& dir_path
	);
}
#pragma once

#include "nt.hpp"
#include <ShlObj.h>
#include <atlbase.h>

namespace utils::com
{
	bool select_folder(std::wstring& out_folder, const std::wstring& title = L"Select a Folder", const std::wstring& selected_folder = {});
	CComPtr<IProgressDialog> create_progress_dialog();
}

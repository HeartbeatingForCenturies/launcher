#pragma once

#include "named_mutex.hpp"
#include <mutex>
#include <optional>
#include <filesystem>

namespace utils::properties
{
	std::filesystem::path get_appdata_path();

	std::unique_lock<named_mutex> lock();

	std::optional<std::wstring> load(const std::wstring& name);
	void store(const std::wstring& name, const std::wstring& value);
}

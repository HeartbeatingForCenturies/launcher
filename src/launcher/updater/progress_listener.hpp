#pragma once

#include "file_info.hpp"

namespace updater
{
	class progress_listener
	{
	public:
		virtual void update_files(const std::vector<file_info>& files) = 0;
		virtual void done_update() = 0;
		
		virtual void begin_file(const file_info& file) = 0;
		virtual void end_file(const file_info& file) = 0;
		
		virtual void file_progress(const file_info& file, size_t progress) = 0;
	};
}

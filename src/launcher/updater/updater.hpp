#pragma once

#include "update_cancelled.hpp"

namespace updater
{
	bool is_main_channel();

	void run(const std::string& base);

	void update_iw4x();
}

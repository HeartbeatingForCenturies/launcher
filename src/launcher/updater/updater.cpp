#include "std_include.hpp"

#include "updater.hpp"
#include "updater_ui.hpp"
#include "file_updater.hpp"

#include <version.hpp>

#include <utils/properties.hpp>

namespace updater
{
	namespace
	{
		bool is_master_branch()
		{
			return GIT_BRANCH == "master"s;
		}

		bool is_channel_switch_to_main()
		{
			return strstr(GetCommandLineA(), "--xlabs-channel-main");
		}

		bool is_channel_switch_to_develop()
		{
			return strstr(GetCommandLineA(), "--xlabs-channel-develop");
		}
	}

	bool is_main_channel()
	{
		static auto result = (is_master_branch() || is_channel_switch_to_main()) && !is_channel_switch_to_develop();
		return result;
	}

	void run(const std::string& base)
	{
		const utils::nt::library self;
		const auto self_file = self.get_path();

		updater_ui updater_ui{};
		const file_updater file_updater{updater_ui, base, self_file};

		file_updater.run();

		std::this_thread::sleep_for(1s);
	}

	void update_iw4x()
	{
		const auto mw2_install = utils::properties::load("mw2-install");
		if (!mw2_install)
		{
			return;
		}

		const auto base = mw2_install.value() + "\\";

		updater_ui updater_ui{};
		const file_updater file_updater{updater_ui, base, ""};
		file_updater.update_iw4x_if_necessary();

		std::this_thread::sleep_for(1s);
	}
}

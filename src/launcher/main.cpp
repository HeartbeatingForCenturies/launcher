#include "std_include.hpp"
#include "cef/cef_ui.hpp"
#include "updater/updater.hpp"

#include <utils/com.hpp>
#include <utils/flags.hpp>
#include <utils/named_mutex.hpp>
#include <utils/exit_callback.hpp>
#include <utils/properties.hpp>
#include <utils/io.hpp>
#include <utils/string.hpp>

namespace
{
	bool try_lock_termination_barrier()
	{
		static std::atomic_bool barrier{false};

		auto expected = false;
		return barrier.compare_exchange_strong(expected, true);
	}

	void set_working_directory()
	{
		const auto appdata = utils::properties::get_appdata_path();
		std::filesystem::current_path(appdata);
	}

	void enable_dpi_awareness()
	{
		const utils::nt::library user32{"user32.dll"};

		const auto set_dpi_awareness_context = user32
			? user32.get_proc<BOOL(WINAPI*)(DPI_AWARENESS_CONTEXT)>("SetProcessDpiAwarenessContext")
			: nullptr;

		// Minimum: Windows 10, version 1703
		if (set_dpi_awareness_context)
		{
			set_dpi_awareness_context(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
			return;
		}

		const utils::nt::library shcore{"shcore.dll"};

		const auto set_dpi_awareness = shcore
			? shcore.get_proc<HRESULT(WINAPI*)(PROCESS_DPI_AWARENESS)>("SetProcessDpiAwareness")
			: nullptr;

		// Minimum: Windows 8.1
		if (set_dpi_awareness)
		{
			set_dpi_awareness(PROCESS_PER_MONITOR_DPI_AWARE);
			return;
		}

		// Call vista function if nothing else was not resolved
		SetProcessDPIAware();
	}

	void run_as_singleton()
	{
		static utils::named_mutex mutex{"xlabs-launcher"};
		if (!mutex.try_lock(3s))
		{
			throw std::runtime_error{"X Labs launcher is already running"};
		}
	}

	bool is_subprocess()
	{
		return strstr(GetCommandLineA(), "--xlabs-subprocess");
	}

	bool is_dedi()
	{
		return !is_subprocess() && (utils::flags::has_flag("dedicated") || utils::flags::has_flag("update"));
	}

	void run_watchdog()
	{
		std::thread([]()
		{
			const auto parent = utils::nt::get_parent_pid();
			if (utils::nt::wait_for_process(parent))
			{
				std::this_thread::sleep_for(3s);
				utils::nt::terminate();
			}
		}).detach();
	}

	int run_subprocess(const utils::nt::library& process, const std::filesystem::path& path)
	{
		const cef::cef_ui cef_ui{process, path};
		return cef_ui.run_process();
	}

	void add_commands(cef::cef_ui& cef_ui)
	{
		cef_ui.add_command("launch-aw", [&cef_ui](const WValue& value, auto&)
		{
			if (!value.IsString())
			{
				return;
			}

			const std::wstring arg{value.GetString()};

			static const std::unordered_map<std::wstring, std::string> arg_mapping = {
				{L"aw-sp", "-singleplayer"},
				{L"aw-mp", "-multiplayer"},
				{L"aw-zm", "-zombies"},
				{L"aw-survival", "-survival"},
			};

			const auto mapped_arg = arg_mapping.find(arg);
			if (mapped_arg == arg_mapping.end())
			{
				return;
			}

			const auto aw_install = utils::properties::load(L"aw-install");
			if (!aw_install)
			{
				return;
			}

			if (!try_lock_termination_barrier())
			{
				return;
			}

			SetEnvironmentVariableW(L"XLABS_AW_INSTALL", aw_install->data());

			const auto s1x_exe = utils::properties::get_appdata_path() / "data" / "s1x" / "s1x.exe";
			utils::nt::launch_process(s1x_exe, mapped_arg->second);

			cef_ui.close_browser();
		});

		cef_ui.add_command("launch-ghosts", [&cef_ui](const WValue& value, auto&)
		{
			if (!value.IsString())
			{
				return;
			}

			const std::wstring arg{value.GetString()};

			static const std::unordered_map<std::wstring, std::string> arg_mapping = {
				{L"ghosts-sp", "-singleplayer"},
				{L"ghosts-mp", "-multiplayer"},
			};

			const auto mapped_arg = arg_mapping.find(arg);
			if (mapped_arg == arg_mapping.end())
			{
				return;
			}

			const auto ghosts_install = utils::properties::load(L"ghosts-install");
			if (!ghosts_install)
			{
				return;
			}

			if (!try_lock_termination_barrier())
			{
				return;
			}

			SetEnvironmentVariableW(L"XLABS_GHOSTS_INSTALL", ghosts_install->data());

			const auto iw6x_exe = utils::properties::get_appdata_path() / "data" / "iw6x" / "iw6x.exe";
			utils::nt::launch_process(iw6x_exe, mapped_arg->second);

			cef_ui.close_browser();
		});

		cef_ui.add_command("launch-mw2", [&cef_ui](const WValue& value, auto&)
		{
			if (!value.IsString())
			{
				return;
			}

			const std::wstring arg{value.GetString()};

			static const std::unordered_map<std::wstring, std::string> arg_mapping = {
				{L"mw2-sp", "-singleplayer"},
				{L"mw2-mp", "-multiplayer"},
			};

			const auto mapped_arg = arg_mapping.find(arg);
			if (mapped_arg == arg_mapping.end())
			{
				return;
			}

			const auto mw2_install = utils::properties::load(L"mw2-install");
			if (!mw2_install)
			{
				return;
			}

			if (!try_lock_termination_barrier())
			{
				return;
			}

			updater::update_iw4x();

			SetEnvironmentVariableW(L"XLABS_MW2_INSTALL", mw2_install->data());

			// Until MP changes it way of loading this is the only way
			if (arg == L"mw2-sp")
			{
				const auto iw4x_sp_exe = utils::properties::get_appdata_path() / "data" / "iw4x" / "iw4x-sp.exe";
				utils::nt::launch_process(iw4x_sp_exe, mapped_arg->second);
			}
			else
			{
				const auto iw4x_exe = mw2_install.value() + L"\\iw4x.exe";
				const auto iw4x_dll = mw2_install.value() + L"\\iw4x.dll";
				const auto search_path = utils::properties::get_appdata_path() / "data" / "iw4x";

				utils::io::remove_file(iw4x_dll);
				utils::nt::update_dll_search_path(search_path);
				utils::nt::launch_process(iw4x_exe, mapped_arg->second);
			}

			cef_ui.close_browser();
		});

		cef_ui.add_command("browse-folder", [](const auto&, WDocument& response)
		{
			response.SetNull();

			std::wstring folder{};
			if (utils::com::select_folder(folder))
			{
				response.SetString(folder, response.GetAllocator());
			}
		});

		cef_ui.add_command("close", [&cef_ui](const auto&, auto&)
		{
			cef_ui.close_browser();
		});

		cef_ui.add_command("minimize", [&cef_ui](const auto&, auto&)
		{
			ShowWindow(cef_ui.get_window(), SW_MINIMIZE);
		});

		cef_ui.add_command("show", [&cef_ui](const auto&, auto&)
		{
			auto* const window = cef_ui.get_window();
			ShowWindow(window, SW_SHOWDEFAULT);
			SetForegroundWindow(window);

			PostMessageA(window, WM_DELAYEDDPICHANGE, 0, 0);
		});

		cef_ui.add_command("get-property", [](const WValue& value, WDocument& response)
		{
			response.SetNull();

			if (!value.IsString())
			{
				return;
			}

			const std::wstring key{value.GetString()};
			const auto property = utils::properties::load(key);
			if (!property)
			{
				return;
			}

			response.SetString(*property, response.GetAllocator());
		});

		cef_ui.add_command("set-property", [](const WValue& value, auto&)
		{
			if (!value.IsObject())
			{
				return;
			}

			const auto _ = utils::properties::lock();

			for (auto i = value.MemberBegin(); i != value.MemberEnd(); ++i)
			{
				if (!i->value.IsString())
				{
					continue;
				}

				const std::wstring key{i->name.GetString()};
				const std::wstring val{i->value.GetString()};

				utils::properties::store(key, val);
			}
		});

		cef_ui.add_command("get-channel", [](auto&, WDocument& response)
		{
			const std::wstring channel = updater::is_main_channel() ? L"main" : L"dev";
			response.SetString(channel, response.GetAllocator());
		});

		cef_ui.add_command("switch-channel", [&cef_ui](const WValue& value, auto&)
		{
			if (!value.IsString())
			{
				return;
			}

			const std::wstring channel{value.GetString()};
			const auto* const command_line = channel == L"main" ? "--xlabs-channel-main" : "--xlabs-channel-develop";

			utils::at_exit([command_line]
			{
				utils::nt::relaunch_self(command_line);
			});

			cef_ui.close_browser();
		});
	}

	void show_window(const utils::nt::library& process, const std::filesystem::path& path)
	{
		cef::cef_ui cef_ui{process, path};
		add_commands(cef_ui);
		cef_ui.create(path / "data" / "launcher-ui", "main.html");
		cef::cef_ui::work();
	}
}

int CALLBACK WinMain(const HINSTANCE instance, HINSTANCE, LPSTR, int)
{
	try
	{
		//set_working_directory();

		const utils::nt::library lib{instance};
		const auto path = utils::properties::get_appdata_path();

		if (is_subprocess())
		{
			run_watchdog();
			return run_subprocess(lib, path);
		}

		enable_dpi_awareness();

#if defined(CI_BUILD) && !defined(DEBUG)
		run_as_singleton();

		if (!utils::flags::has_flag("noupdate"))
		{
			updater::run(path);
			updater::update_iw4x();
		}
#endif

		if (!is_dedi())
		{
			show_window(lib, path);
		}

		return 0;
	}
	catch (updater::update_cancelled&)
	{
		return 0;
	}
	catch (std::exception& e)
	{
		MessageBoxA(nullptr, e.what(), "ERROR", MB_ICONERROR);
	}
	catch (...)
	{
		MessageBoxA(nullptr, "An unknown error occurred", "ERROR", MB_ICONERROR);
	}

	return 1;
}

#pragma once

#include <string>

namespace mameLib
{
	// Where MAME finds ROMs and keeps its data. Read from mame-plugin.ini in the user's configuration directory
	// (rompath=, datapath=, layoutdev=), MAME_PLUGIN_ROMPATH, MAME_PLUGIN_DATAPATH and MAME_PLUGIN_LAYOUTDEV override
	// it. The same file is used by the CLAP plugin of the MAME tree, so both find the same ROMs
	struct Settings
	{
		std::string rompath;	// ';'-separated, searched after <datapath>/<machine>/roms
		std::string datapath;	// a folder per machine: roms, NVRAM templates in nvram, panels for CtrlrX, layout files
		bool layoutDev = false;	// load <datapath>/<machine>/default.lay again whenever it changes

		static Settings load();
	};
}

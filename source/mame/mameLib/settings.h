#pragma once

#include <string>

namespace mameLib
{
	// Where MAME finds ROMs and keeps its data. Read from mame-plugin.ini in the user's configuration directory
	// (rompath=, datapath=), MAME_PLUGIN_ROMPATH and MAME_PLUGIN_DATAPATH override it. The same file is used by the
	// CLAP plugin of the MAME tree, so both find the same ROMs
	struct Settings
	{
		std::string rompath;	// ';'-separated, searched after <datapath>/<machine>/roms
		std::string datapath;	// a folder per machine: roms, NVRAM templates in nvram, panels for CtrlrX

		static Settings load();
	};
}

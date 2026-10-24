#pragma once

#include <string>

namespace mameLib
{
	// Where MAME finds ROMs and keeps its data. Read from mame-plugin.ini in the user's configuration directory
	// (rompath=, datapath=), MAME_PLUGIN_ROMPATH and MAME_PLUGIN_DATAPATH override it. The same file is used by the
	// CLAP plugin of the MAME tree, so both find the same ROMs
	struct Settings
	{
		std::string rompath;	// ';'-separated
		std::string datapath;	// NVRAM files in <datapath>/<machine>/nvram are the template for new instances

		static Settings load();
	};
}

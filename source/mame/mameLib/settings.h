#pragma once

#include <string>

namespace mameLib
{
	// Where MAME finds ROMs and keeps its data. Read from mame-plugin.ini in the user's configuration directory
	// (rompath=, datapath=, layoutdev=, nativelayout=, uiscale=), MAME_PLUGIN_ROMPATH, MAME_PLUGIN_DATAPATH,
	// MAME_PLUGIN_LAYOUTDEV, MAME_PLUGIN_NATIVELAYOUT and MAME_PLUGIN_UISCALE override it. The same file is used by the CLAP plugin of the MAME tree, so both find the same ROMs
	struct Settings
	{
		std::string rompath;	// ';'-separated, searched after <datapath>/<machine>/roms
		std::string datapath;	// a folder per machine: roms, NVRAM templates in nvram, panels for CtrlrX, layout files
		bool layoutDev = false;	// load <datapath>/<machine>/default.lay again whenever it changes
		bool nativeLayout = false;	// the editor draws the layout itself instead of showing MAME's rendering
		int uiScale = 0;			// the editor's scale in percent for all the plugins, 0 leaves it to each plugin's own

		static Settings load();
	};
}

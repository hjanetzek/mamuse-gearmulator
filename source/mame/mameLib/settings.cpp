#include "settings.h"

#include <cstdlib>
#include <fstream>
#include <string_view>

namespace mameLib
{
	namespace
	{
		std::string trim(const std::string_view _text)
		{
			const auto first = _text.find_first_not_of(" \t\r\n");
			if (first == std::string_view::npos)
				return {};
			const auto last = _text.find_last_not_of(" \t\r\n");
			return std::string(_text.substr(first, last - first + 1));
		}

		std::string envOr(const char* _name, std::string _fallback)
		{
			const char* value = std::getenv(_name);
			return value && *value ? std::string(value) : std::move(_fallback);
		}
	}

	Settings Settings::load()
	{
#if defined(_WIN32)
		const std::string configDir = envOr("APPDATA", ".") + "\\mame-plugin";
		const std::string dataDir = configDir;
		const char sep = '\\';
#elif defined(__APPLE__)
		const std::string configDir = envOr("HOME", ".") + "/Library/Application Support/mame-plugin";
		const std::string dataDir = configDir;
		const char sep = '/';
#else
		const std::string home = envOr("HOME", ".");
		const std::string configDir = envOr("XDG_CONFIG_HOME", home + "/.config") + "/mame-plugin";
		const std::string dataDir = envOr("XDG_DATA_HOME", home + "/.local/share") + "/mame-plugin";
		const char sep = '/';
#endif

		Settings result;
		result.datapath = dataDir;

		std::ifstream file(configDir + sep + "mame-plugin.ini");
		std::string line;
		while (std::getline(file, line))
		{
			const auto eq = line.find('=');
			if (eq == std::string::npos || line[0] == '#' || line[0] == ';')
				continue;
			const auto key = trim(std::string_view(line).substr(0, eq));
			auto value = trim(std::string_view(line).substr(eq + 1));
			if (key == "rompath")
				result.rompath = std::move(value);
			else if (key == "datapath")
				result.datapath = std::move(value);
		}

		result.rompath = envOr("MAME_PLUGIN_ROMPATH", std::move(result.rompath));
		result.datapath = envOr("MAME_PLUGIN_DATAPATH", std::move(result.datapath));
		return result;
	}
}

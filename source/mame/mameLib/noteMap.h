#pragma once

#include <string>
#include <vector>

#include "mame_plugin.h"

namespace mameLib
{
	// MIDI notes pressing the inputs of a machine without MIDI input, e.g. the pads of a drum machine. Empty for other
	// machines
	const std::vector<mp_note_mapping>& getNoteMap(const std::string& _machine);
}

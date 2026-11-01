#include "noteMap.h"

namespace mameLib
{
	namespace
	{
		// General MIDI drum notes on the LinnDrum's pads, the input names are those of linn/linndrum.cpp. Velocity picks
		// the louder variations. The upper row of pads plays either drums (hi-hats, toms, rides) or percussion (cabasa,
		// tambourine, congas, cowbell, claps), the PERC. button switches between them; the notes of both go to the same pads
		const std::vector<mp_note_mapping> g_linndrum =
		{
			{35, 0, "BASS 1"}, {35, 64, "BASS 2"},
			{36, 0, "BASS 1"}, {36, 64, "BASS 2"},
			{37, 0, "SIDESTICK"},
			{38, 0, "SNARE 1"}, {38, 43, "SNARE 2"}, {38, 86, "SNARE 3"},
			{40, 0, "SNARE 1"}, {40, 43, "SNARE 2"}, {40, 86, "SNARE 3"},
			{49, 0, "CRASH"},
			{57, 0, "CRASH"},

			// drums
			{42, 0, "CABASA1 / HIHAT1"}, {42, 64, "CABASA2 / HIHAT2"},
			{44, 0, "CABASA1 / HIHAT1"}, {44, 64, "CABASA2 / HIHAT2"},
			{46, 0, "TAMP 1 / HIHAT O"},
			{48, 0, "TAMP 2 / HI TOM"},
			{50, 0, "TAMP 2 / HI TOM"},
			{45, 0, "HI CONGA / MID TOM"},
			{47, 0, "HI CONGA / MID TOM"},
			{41, 0, "LO CONGA / LO TOM"},
			{43, 0, "LO CONGA / LO TOM"},
			{51, 0, "COWBELL / RIDE 1"},
			{53, 0, "CLAPS / RIDE 2"},
			{59, 0, "CLAPS / RIDE 2"},

			// percussion
			{69, 0, "CABASA1 / HIHAT1"}, {69, 64, "CABASA2 / HIHAT2"},
			{54, 0, "TAMP 1 / HIHAT O"}, {54, 64, "TAMP 2 / HI TOM"},
			{62, 0, "HI CONGA / MID TOM"},
			{63, 0, "HI CONGA / MID TOM"},
			{64, 0, "LO CONGA / LO TOM"},
			{56, 0, "COWBELL / RIDE 1"},
			{39, 0, "CLAPS / RIDE 2"},
		};
	}

	const std::vector<mp_note_mapping>& getNoteMap(const std::string& _machine)
	{
		static const std::vector<mp_note_mapping> none;

		if (_machine == "linndrum")
			return g_linndrum;
		return none;
	}
}

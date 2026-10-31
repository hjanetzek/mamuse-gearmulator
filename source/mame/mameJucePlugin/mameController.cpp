#include "mameController.h"

#include "mamePluginProcessor.h"

#if MAME_WITH_CTRLR
#include "mameCtrlr.h"
#endif

namespace mameJucePlugin
{
	Controller::Controller(AudioPluginAudioProcessor& _processor) : pluginLib::Controller(_processor, "parameterDescriptions_mame.json")
	{
		registerParams(_processor, nullptr);
	}

	Controller::~Controller() = default;

	bool Controller::parseMidiMessage(const synthLib::SMidiEvent& _e)
	{
#if MAME_WITH_CTRLR
		// the machine's output, e.g. the patch dumps a panel asks for
		if (_e.source == synthLib::MidiEventSource::Device)
			if (auto* ctrlr = static_cast<AudioPluginAudioProcessor&>(getProcessor()).getCtrlr())
				ctrlr->receive(_e);
#endif
		return pluginLib::Controller::parseMidiMessage(_e);
	}
}

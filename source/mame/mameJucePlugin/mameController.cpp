#include "mameController.h"

#include "mamePluginProcessor.h"

namespace mameJucePlugin
{
	Controller::Controller(AudioPluginAudioProcessor& _processor) : pluginLib::Controller(_processor, "parameterDescriptions_mame.json")
	{
		registerParams(_processor, nullptr);
	}

	Controller::~Controller() = default;
}

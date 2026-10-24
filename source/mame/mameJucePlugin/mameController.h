#pragma once

#include "jucePluginLib/controller.h"

namespace mameJucePlugin
{
	class AudioPluginAudioProcessor;

	// no parameters yet: the machine is edited through its own front panel or a SysEx editor
	class Controller : public pluginLib::Controller
	{
	public:
		explicit Controller(AudioPluginAudioProcessor& _processor);
		~Controller() override;

		void sendParameterChange(const pluginLib::Parameter& _parameter, pluginLib::ParamValue _value, pluginLib::Parameter::Origin _origin) override {}
		bool parseSysexMessage(const pluginLib::SysEx&, synthLib::MidiEventSource) override { return false; }
		void onStateLoaded() override {}
	};
}

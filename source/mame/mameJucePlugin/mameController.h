#pragma once

#include <functional>
#include <string>
#include <vector>

#include "jucePluginLib/controller.h"

namespace mameJucePlugin
{
	class AudioPluginAudioProcessor;

	// The parameters are the machine's adjusters, e.g. a drum machine's knobs and mixer, named as MAME names them
	// (parameterDescriptions_mame.json of the product, written by mpctest with MPCTEST_FIELDS). Their values live in
	// the machine and are saved with its NVRAM; the controller passes changes to it and reads its values back, which
	// also change when the panel's knobs are dragged. Everything else is edited through the machine's own front panel
	// or a SysEx editor
	class Controller : public pluginLib::Controller
	{
	public:
		explicit Controller(AudioPluginAudioProcessor& _processor);
		~Controller() override;

		void sendParameterChange(const pluginLib::Parameter& _parameter, pluginLib::ParamValue _value, pluginLib::Parameter::Origin _origin) override;
		bool parseSysexMessage(const pluginLib::SysEx&, synthLib::MidiEventSource) override { return false; }
		bool parseMidiMessage(const synthLib::SMidiEvent& _e) override;
		void onStateLoaded() override {}

	private:
		// the base class is a juce::Timer already
		class FieldTimer : public juce::Timer
		{
		public:
			explicit FieldTimer(std::function<void()> _callback) : m_callback(std::move(_callback)) {}
			void timerCallback() override { m_callback(); }
		private:
			std::function<void()> m_callback;
		};

		void readFields();

		AudioPluginAudioProcessor& m_mameProcessor;
		FieldTimer m_fieldTimer;
		uint32_t m_fieldGeneration = ~0u;
		uint64_t m_fieldSerial = 0;
		std::vector<std::string> m_fieldNames;
		std::vector<int32_t> m_fieldValues;
	};
}

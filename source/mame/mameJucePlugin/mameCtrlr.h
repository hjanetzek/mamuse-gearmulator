#pragma once

#include <memory>
#include <vector>

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_gui_basics/juce_gui_basics.h>

class CtrlrProcessor;

namespace synthLib
{
	struct SMidiEvent;
}

namespace mameJucePlugin
{
	class AudioPluginAudioProcessor;

	// Runs CtrlrX panels for the machine (see docs/ctrlrx.md of muse). The panels' MIDI goes to the machine and the
	// machine's MIDI to the panels. Everything here runs on the message thread
	class Ctrlr : juce::Timer
	{
	public:
		explicit Ctrlr(AudioPluginAudioProcessor& _processor);
		~Ctrlr() override;

		Ctrlr(const Ctrlr&) = delete;
		Ctrlr& operator = (const Ctrlr&) = delete;

		// the machine's MIDI output
		void receive(const synthLib::SMidiEvent& _event);

		// opens the window with the CtrlrX editor. Without a panel, the machine's default panel is opened, the first
		// panel file in <datapath>/<machine>/panels
		void showEditor();

		void saveState(std::vector<uint8_t>& _data) const;
		void loadState(const std::vector<uint8_t>& _data);

	private:
		void timerCallback() override;
		void openDefaultPanel() const;
		void setMidiOptions() const;

		AudioPluginAudioProcessor& m_processor;
		std::unique_ptr<CtrlrProcessor> m_ctrlr;
		std::unique_ptr<juce::DocumentWindow> m_window;

		// the machine's MIDI for the panels, and the panels' MIDI for the machine
		juce::MidiBuffer m_toPanels;
		juce::MidiBuffer m_fromPanels;
		juce::AudioBuffer<float> m_silence;
	};
}

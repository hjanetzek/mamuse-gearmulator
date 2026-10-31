#include "mameCtrlr.h"

#include "mamePluginProcessor.h"

#include "mameLib/settings.h"

#include "synthLib/midiTypes.h"

// CtrlrX, with JUCE in the global namespace
#include "stdafx.h"
#include "CtrlrProcessor.h"
#include "CtrlrManager/CtrlrManager.h"
#include "CtrlrPanel/CtrlrPanel.h"
#include "CtrlrPanel/CtrlrPanelEditor.h"

namespace mameJucePlugin
{
	namespace
	{
		// how often the panels' MIDI is passed to the machine
		constexpr int g_timerMs = 5;

		class Window final : public juce::DocumentWindow
		{
		public:
			explicit Window(const juce::String& _title) : DocumentWindow(_title, juce::Colours::darkgrey, allButtons)
			{
				setUsingNativeTitleBar(true);
				setResizable(true, false);
			}

			void closeButtonPressed() override
			{
				setVisible(false);
			}
		};
	}

	Ctrlr::Ctrlr(AudioPluginAudioProcessor& _processor) : m_processor(_processor), m_ctrlr(std::make_unique<CtrlrProcessor>())
	{
		m_silence.setSize(2, 64);
		startTimer(g_timerMs);
	}

	Ctrlr::~Ctrlr()
	{
		stopTimer();
		// the editor belongs to the processor, it goes first
		m_window.reset();
		m_ctrlr.reset();
	}

	void Ctrlr::receive(const synthLib::SMidiEvent& _event)
	{
		if (!_event.sysex.empty())
			m_toPanels.addEvent(_event.sysex.data(), static_cast<int>(_event.sysex.size()), 0);
		else
			m_toPanels.addEvent(juce::MidiMessage(_event.a, _event.b, _event.c), 0);
	}

	void Ctrlr::showEditor()
	{
		if (!m_window)
		{
			m_window = std::make_unique<Window>(juce::String(JucePlugin_Name) + " - CtrlrX");
			m_window->setContentOwned(m_ctrlr->createEditorIfNeeded(), true);
			m_window->centreWithSize(m_window->getWidth(), m_window->getHeight());
		}

		// after the editor, CtrlrX expects it to be there when a panel is opened
		if (m_ctrlr->getManager().getNumPanels() == 0)
			openDefaultPanel();

		m_window->setVisible(true);
		m_window->toFront(true);
	}

	void Ctrlr::saveState(std::vector<uint8_t>& _data) const
	{
		juce::MemoryBlock state;
		m_ctrlr->getStateInformation(state);
		_data.assign(static_cast<const uint8_t*>(state.getData()), static_cast<const uint8_t*>(state.getData()) + state.getSize());
	}

	void Ctrlr::loadState(const std::vector<uint8_t>& _data)
	{
		if (_data.empty())
			return;

		// CtrlrX adds the panels of the state to those that are open
		auto& manager = m_ctrlr->getManager();
		while (manager.getNumPanels() > 0)
			manager.removePanel(manager.getPanel(0)->getEditor(true));

		m_ctrlr->setStateInformation(_data.data(), static_cast<int>(_data.size()));
	}

	void Ctrlr::timerCallback()
	{
		if (!m_toPanels.isEmpty())
		{
			// not through processBlock(), which passes its input on to its output
			const juce::AudioPlayHead::CurrentPositionInfo position;
			m_ctrlr->processPanels(m_toPanels, position);
			m_toPanels.clear();
		}

		m_fromPanels.clear();
		m_ctrlr->processBlock(m_silence, m_fromPanels);

		for (const auto meta : m_fromPanels)
		{
			const auto* data = meta.data;
			const auto size = meta.numBytes;

			if (size <= 0)
				continue;

			synthLib::SMidiEvent ev(synthLib::MidiEventSource::Editor);

			if (data[0] == 0xf0)
			{
				ev.sysex.assign(data, data + size);
			}
			else
			{
				ev.a = data[0];
				ev.b = size > 1 ? data[1] : 0;
				ev.c = size > 2 ? data[2] : 0;
			}

			m_processor.addMidiEvent(ev);
		}
	}

	void Ctrlr::openDefaultPanel() const
	{
		const auto settings = mameLib::Settings::load();
		const juce::File dir(juce::String(settings.datapath) + "/" + MAME_MACHINE + "/panels");

		auto files = dir.findChildFiles(juce::File::findFiles, false, "*.bpanelz;*.bpanel;*.panelz;*.panel");
		if (files.isEmpty())
			return;

		files.sort();
		m_ctrlr->openFileFromCli(files.getFirst());
	}
}

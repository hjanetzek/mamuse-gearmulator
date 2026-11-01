#include "mameController.h"

#include "mamePluginProcessor.h"

#include "mameLib/panel.h"

#if MAME_WITH_CTRLR
#include "mameCtrlr.h"
#endif

namespace mameJucePlugin
{
	namespace
	{
		// how often the machine's adjusters are read back, e.g. while a knob is dragged on the panel
		constexpr int g_fieldReadsPerSecond = 30;
	}

	Controller::Controller(AudioPluginAudioProcessor& _processor)
		: pluginLib::Controller(_processor, "parameterDescriptions_mame.json")
		, m_mameProcessor(_processor)
		, m_fieldTimer([this] { readFields(); })
	{
		registerParams(_processor, nullptr);
		m_fieldTimer.startTimerHz(g_fieldReadsPerSecond);
	}

	Controller::~Controller()
	{
		m_fieldTimer.stopTimer();
	}

	void Controller::sendParameterChange(const pluginLib::Parameter& _parameter, const pluginLib::ParamValue _value, pluginLib::Parameter::Origin)
	{
		m_mameProcessor.getPanel().setField(_parameter.getDescription().name, _value);
	}

	void Controller::readFields()
	{
		auto& panel = m_mameProcessor.getPanel();

		// another instance of the machine, e.g. after a state was loaded, starts its serials again
		if (panel.getGeneration() != m_fieldGeneration)
		{
			m_fieldGeneration = panel.getGeneration();
			m_fieldSerial = 0;
			m_fieldNames = panel.getFieldNames();
		}

		const auto serial = panel.getFieldValues(m_fieldSerial, m_fieldValues);
		if (serial == m_fieldSerial)
			return;
		m_fieldSerial = serial;

		for (size_t i = 0; i < m_fieldNames.size() && i < m_fieldValues.size(); ++i)
		{
			if (auto* parameter = getParameter(m_fieldNames[i], 0))
				parameter->setValueFromSynth(m_fieldValues[i], pluginLib::Parameter::Origin::Midi);
		}
	}

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

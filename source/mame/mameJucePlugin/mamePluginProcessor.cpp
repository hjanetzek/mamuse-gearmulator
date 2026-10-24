#include "mamePluginProcessor.h"

#include "mameController.h"

#include "jucePluginEditorLib/pluginEditor.h"
#include "jucePluginEditorLib/pluginEditorState.h"
#include "jucePluginEditorLib/pluginEntry.h"

#include "mameLib/device.h"

#include "skins.h"

// before processorPropertiesInit.h, which refers to it
#include "BinaryData.h"
#include "jucePluginLib/processorPropertiesInit.h"

namespace mameJucePlugin
{
	AudioPluginAudioProcessor::AudioPluginAudioProcessor() :
		Processor(BusesProperties()
			.withOutput("Out", juce::AudioChannelSet::stereo(), true)
			, {}, pluginLib::initProcessorProperties())
	{
	}

	jucePluginEditorLib::PluginEditorState* AudioPluginAudioProcessor::createEditorState()
	{
		return new jucePluginEditorLib::PluginEditorStateT<jucePluginEditorLib::Editor>(*this, g_includedSkins);
	}

	synthLib::Device* AudioPluginAudioProcessor::newDevice(const synthLib::DeviceCreateParams& _params)
	{
		return new mameLib::Device(_params);
	}

	void AudioPluginAudioProcessor::getRemoteDeviceParams(synthLib::DeviceCreateParams& _params) const
	{
		Processor::getRemoteDeviceParams(_params);
		_params.romName = MAME_MACHINE;
	}

	pluginLib::Controller* AudioPluginAudioProcessor::createController()
	{
		return new Controller(*this);
	}

	void AudioPluginAudioProcessor::setNonRealtime(const bool _isNonRealtime) noexcept
	{
		Processor::setNonRealtime(_isNonRealtime);
		if (auto* device = dynamic_cast<mameLib::Device*>(getPlugin().getDevice()))
			device->setOffline(_isNonRealtime);
	}
}

TUS_PLUGIN_ENTRY(mameJucePlugin::AudioPluginAudioProcessor)

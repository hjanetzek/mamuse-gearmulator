#pragma once

#include "jucePluginEditorLib/pluginProcessor.h"

#include <memory>

namespace mameLib
{
	class Panel;
}

namespace mameJucePlugin
{
	class AudioPluginAudioProcessor : public jucePluginEditorLib::Processor
	{
	public:
		AudioPluginAudioProcessor();

		jucePluginEditorLib::PluginEditorState* createEditorState() override;
		synthLib::Device* newDevice(const synthLib::DeviceCreateParams& _params) override;
		void getRemoteDeviceParams(synthLib::DeviceCreateParams& _params) const override;
		pluginLib::Controller* createController() override;

		// offline renders wait for the emulation instead of underrunning
		void setNonRealtime(bool _isNonRealtime) noexcept override;

		// the machine's front panel, kept across restarts of the machine
		mameLib::Panel& getPanel() const { return *m_panel; }

	private:
		std::shared_ptr<mameLib::Panel> m_panel;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessor)
	};
}

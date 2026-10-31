#pragma once

#include "jucePluginEditorLib/pluginProcessor.h"

#include <memory>

namespace mameLib
{
	class Panel;
}

namespace mameJucePlugin
{
	class Ctrlr;

	class AudioPluginAudioProcessor : public jucePluginEditorLib::Processor
	{
	public:
		AudioPluginAudioProcessor();
		~AudioPluginAudioProcessor() override;

		jucePluginEditorLib::PluginEditorState* createEditorState() override;
		synthLib::Device* newDevice(const synthLib::DeviceCreateParams& _params) override;
		void getRemoteDeviceParams(synthLib::DeviceCreateParams& _params) const override;
		pluginLib::Controller* createController() override;

		// offline renders wait for the emulation instead of underrunning
		void setNonRealtime(bool _isNonRealtime) noexcept override;

		// the machine's front panel, kept across restarts of the machine
		mameLib::Panel& getPanel() const { return *m_panel; }

		// the CtrlrX panels, null if the plugin is built without CtrlrX
#if MAME_WITH_CTRLR
		Ctrlr* getCtrlr() const { return m_ctrlr.get(); }
#else
		Ctrlr* getCtrlr() const { return nullptr; }
#endif

		void saveChunkData(baseLib::BinaryStream& _s) override;
		void loadChunkData(baseLib::ChunkReader& _cr) override;

	private:
		std::shared_ptr<mameLib::Panel> m_panel;
#if MAME_WITH_CTRLR
		std::unique_ptr<Ctrlr> m_ctrlr;
#endif

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessor)
	};
}

#include "mamePluginProcessor.h"

#include "mameController.h"
#include "mameEditor.h"

#if MAME_WITH_CTRLR
#include "mameCtrlr.h"
#endif

#include "baseLib/binarystream.h"

#include "jucePluginEditorLib/pluginEditor.h"
#include "jucePluginEditorLib/pluginEditorState.h"
#include "jucePluginEditorLib/pluginEntry.h"

#include "juceRmlUi/rmlMenu.h"

#include "mameLib/device.h"
#include "mameLib/panel.h"

#include "skins.h"

// before processorPropertiesInit.h, which refers to it
#include "BinaryData.h"
#include "jucePluginLib/processorPropertiesInit.h"

namespace mameJucePlugin
{
	namespace
	{
		class EditorState final : public jucePluginEditorLib::PluginEditorState
		{
		public:
			explicit EditorState(AudioPluginAudioProcessor& _processor)
				: PluginEditorState(_processor, _processor.getController(), g_includedSkins)
			{
				loadDefaultSkin();
			}

		private:
			jucePluginEditorLib::Editor* createEditor(const jucePluginEditorLib::Skin& _skin) override
			{
				return new Editor(m_processor, _skin);
			}

			void initContextMenu(juceRmlUi::Menu& _menu) override
			{
#if MAME_WITH_CTRLR
				if (auto* ctrlr = static_cast<AudioPluginAudioProcessor&>(m_processor).getCtrlr())
				{
					_menu.addSeparator();
					_menu.addEntry("CtrlrX Panel...", [ctrlr] { ctrlr->showEditor(); });
				}
#endif
			}
		};
	}

	AudioPluginAudioProcessor::AudioPluginAudioProcessor() :
		Processor(BusesProperties()
			.withOutput("Out", juce::AudioChannelSet::stereo(), true)
			, {}, pluginLib::initProcessorProperties())
		, m_panel(std::make_shared<mameLib::Panel>())
	{
#if MAME_WITH_CTRLR
		m_ctrlr = std::make_unique<Ctrlr>(*this);
#endif
	}

	AudioPluginAudioProcessor::~AudioPluginAudioProcessor() = default;

	jucePluginEditorLib::PluginEditorState* AudioPluginAudioProcessor::createEditorState()
	{
		return new EditorState(*this);
	}

	synthLib::Device* AudioPluginAudioProcessor::newDevice(const synthLib::DeviceCreateParams& _params)
	{
		return new mameLib::Device(_params, m_panel);
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

	void AudioPluginAudioProcessor::saveChunkData(baseLib::BinaryStream& _s)
	{
		Processor::saveChunkData(_s);

#if MAME_WITH_CTRLR
		std::vector<uint8_t> state;
		m_ctrlr->saveState(state);

		baseLib::ChunkWriter cw(_s, "CTRL", 1);
		_s.write(state);
#endif
	}

	void AudioPluginAudioProcessor::loadChunkData(baseLib::ChunkReader& _cr)
	{
		Processor::loadChunkData(_cr);

#if MAME_WITH_CTRLR
		_cr.add("CTRL", 1, [this](baseLib::BinaryStream& _s, uint32_t)
		{
			std::vector<uint8_t> state;
			_s.read(state);
			m_ctrlr->loadState(state);
		});
#endif
	}

	void AudioPluginAudioProcessor::setNonRealtime(const bool _isNonRealtime) noexcept
	{
		Processor::setNonRealtime(_isNonRealtime);
		if (auto* device = dynamic_cast<mameLib::Device*>(getPlugin().getDevice()))
			device->setOffline(_isNonRealtime);
	}
}

TUS_PLUGIN_ENTRY(mameJucePlugin::AudioPluginAudioProcessor)

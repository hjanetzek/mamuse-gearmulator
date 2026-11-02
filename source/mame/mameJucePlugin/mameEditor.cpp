#include "mameEditor.h"

#include "mameLayout.h"
#include "mamePanel.h"
#include "mamePluginProcessor.h"

#include "mameLib/settings.h"

#include "juceRmlUi/rmlElemCanvas.h"

namespace mameJucePlugin
{
	Editor::Editor(jucePluginEditorLib::Processor& _processor, const jucePluginEditorLib::Skin& _skin)
		: jucePluginEditorLib::Editor(_processor, _skin)
	{
	}

	Editor::~Editor()
	{
		m_layout.reset();
		m_panel.reset();
	}

	void Editor::create()
	{
		jucePluginEditorLib::Editor::create();

		if (auto* canvas = findChild<juceRmlUi::ElemCanvas>("panel", false))
		{
			auto& panel = dynamic_cast<AudioPluginAudioProcessor&>(getProcessor()).getPanel();
			if (mameLib::Settings::load().nativeLayout)
				m_layout.reset(new LayoutView(panel, getProcessor().getController(), canvas));
			else
				m_panel.reset(new PanelView(panel, canvas));
		}
	}
}

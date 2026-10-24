#include "mameEditor.h"

#include "mamePanel.h"
#include "mamePluginProcessor.h"

#include "juceRmlUi/rmlElemCanvas.h"

namespace mameJucePlugin
{
	Editor::Editor(jucePluginEditorLib::Processor& _processor, const jucePluginEditorLib::Skin& _skin)
		: jucePluginEditorLib::Editor(_processor, _skin)
	{
	}

	Editor::~Editor()
	{
		m_panel.reset();
	}

	void Editor::create()
	{
		jucePluginEditorLib::Editor::create();

		if (auto* canvas = findChild<juceRmlUi::ElemCanvas>("panel", false))
			m_panel.reset(new PanelView(dynamic_cast<AudioPluginAudioProcessor&>(getProcessor()).getPanel(), canvas));
	}
}

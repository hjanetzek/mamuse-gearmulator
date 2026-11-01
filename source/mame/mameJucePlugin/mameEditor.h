#pragma once

#include "jucePluginEditorLib/pluginEditor.h"

namespace mameJucePlugin
{
	class LayoutView;
	class PanelView;

	class Editor final : public jucePluginEditorLib::Editor
	{
	public:
		Editor(jucePluginEditorLib::Processor& _processor, const jucePluginEditorLib::Skin& _skin);
		~Editor() override;

		Editor(Editor&&) = delete;
		Editor(const Editor&) = delete;
		Editor& operator = (Editor&&) = delete;
		Editor& operator = (const Editor&) = delete;

		void create() override;

	private:
		// the machine's panel, as MAME renders it or drawn from its layout (nativelayout in mame-plugin.ini)
		std::unique_ptr<PanelView> m_panel;
		std::unique_ptr<LayoutView> m_layout;
	};
}

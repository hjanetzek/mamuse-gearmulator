#pragma once

#include "jucePluginEditorLib/pluginEditor.h"

namespace mameJucePlugin
{
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
		std::unique_ptr<PanelView> m_panel;
	};
}

#pragma once

#include <cstdint>
#include <vector>

#include "juce_events/juce_events.h"

namespace Rml
{
	class Event;
}

namespace juceRmlUi
{
	class ElemCanvas;
}

namespace mameLib
{
	class Panel;
}

namespace mameJucePlugin
{
	// Shows the machine's front panel, as MAME draws it from its layout, in a canvas of the skin. MAME renders at the
	// canvas' size in pixels, clicks go to the controls of the layout
	class PanelView : juce::Timer
	{
	public:
		PanelView(mameLib::Panel& _panel, juceRmlUi::ElemCanvas* _canvas);
		~PanelView() override;

		PanelView(const PanelView&) = delete;
		PanelView& operator = (const PanelView&) = delete;

	private:
		void timerCallback() override;
		void onMouse(Rml::Event& _event, bool _pressed);
		void onMouseLeave();

		mameLib::Panel& m_panel;
		juceRmlUi::ElemCanvas* m_canvas;
		std::vector<uint8_t> m_frame;
		uint32_t m_width = 0;
		uint32_t m_height = 0;
		bool m_pressed = false;
	};
}

#include "mamePanel.h"

#include <cmath>
#include <cstring>

#include "mameLib/panel.h"

#include "juceRmlUi/rmlElemCanvas.h"
#include "juceRmlUi/rmlEventListener.h"
#include "juceRmlUi/rmlHelper.h"

namespace mameJucePlugin
{
	namespace
	{
		// MAME prepares up to 60 frames a second, drawing them takes a few ms each on this thread
		constexpr int g_framesPerSecond = 30;
	}

	PanelView::PanelView(mameLib::Panel& _panel, juceRmlUi::ElemCanvas* _canvas) : m_panel(_panel), m_canvas(_canvas)
	{
		m_canvas->setRepaintCallback([this](std::vector<uint8_t>& _buffer)
		{
			// the texture may differ from the frame's size if the renderer is limited to power of two sizes
			if (_buffer.size() == m_frame.size())
				std::memcpy(_buffer.data(), m_frame.data(), m_frame.size());
			else
				std::fill(_buffer.begin(), _buffer.end(), 0);
		});

		juceRmlUi::EventListener::Add(m_canvas, Rml::EventId::Mousedown, [this](Rml::Event& _event)
		{
			if (juceRmlUi::helper::getMouseButton(_event) == juceRmlUi::MouseButton::Left)
				onMouse(_event, true);
		});
		juceRmlUi::EventListener::Add(m_canvas, Rml::EventId::Mouseup, [this](Rml::Event& _event)
		{
			if (juceRmlUi::helper::getMouseButton(_event) == juceRmlUi::MouseButton::Left)
				onMouse(_event, false);
		});
		juceRmlUi::EventListener::Add(m_canvas, Rml::EventId::Mousemove, [this](Rml::Event& _event)
		{
			onMouse(_event, m_pressed);
		});
		juceRmlUi::EventListener::Add(m_canvas, Rml::EventId::Mouseout, [this](Rml::Event&)
		{
			onMouseLeave();
		});

		startTimerHz(g_framesPerSecond);
	}

	PanelView::~PanelView()
	{
		stopTimer();
		m_panel.setSize(0, 0);
	}

	void PanelView::timerCallback()
	{
		// render at the canvas' size in pixels
		const auto size = m_canvas->GetBox().GetSize(Rml::BoxArea::Content);
		const auto width = static_cast<uint32_t>(std::lround(size.x));
		const auto height = static_cast<uint32_t>(std::lround(size.y));

		if (width != m_width || height != m_height)
		{
			m_width = width;
			m_height = height;
			m_panel.setSize(width, height);
			m_canvas->setFixedTextureSize(static_cast<int>(width), static_cast<int>(height));
		}

		if (m_panel.getFrame(m_frame))
			m_canvas->repaint();
	}

	void PanelView::onMouse(Rml::Event& _event, const bool _pressed)
	{
		if (!m_width || !m_height)
			return;

		const auto pos = juceRmlUi::helper::getMousePos(_event) - m_canvas->GetAbsoluteOffset(Rml::BoxArea::Content);
		m_pressed = _pressed;
		m_panel.pointer(0, static_cast<int32_t>(pos.x), static_cast<int32_t>(pos.y), _pressed);
	}

	void PanelView::onMouseLeave()
	{
		m_pressed = false;
		m_panel.pointerLeave(0);
	}
}

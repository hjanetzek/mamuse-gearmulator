#include "panel.h"

namespace mameLib
{
	void Panel::attach(mp_instance* _instance)
	{
		std::lock_guard lock(m_mutex);
		m_instance = _instance;
		m_serial = 0;
		if (m_instance && m_width && m_height)
			mp_panel_set_size(m_instance, m_width, m_height);
	}

	void Panel::detach()
	{
		std::lock_guard lock(m_mutex);
		m_instance = nullptr;
	}

	bool Panel::getNaturalSize(uint32_t& _width, uint32_t& _height)
	{
		std::lock_guard lock(m_mutex);
		if (!m_instance)
			return false;
		mp_panel_size(m_instance, &_width, &_height);
		return _width && _height;
	}

	void Panel::setSize(const uint32_t _width, const uint32_t _height)
	{
		std::lock_guard lock(m_mutex);
		if (_width == m_width && _height == m_height)
			return;
		m_width = _width;
		m_height = _height;
		m_serial = 0;
		if (m_instance)
			mp_panel_set_size(m_instance, _width, _height);
	}

	bool Panel::getFrame(std::vector<uint8_t>& _rgba)
	{
		std::lock_guard lock(m_mutex);
		if (!m_instance || !m_width || !m_height)
			return false;
		_rgba.resize(static_cast<size_t>(m_width) * m_height * 4);
		const auto serial = mp_panel_frame(m_instance, _rgba.data(), m_width, m_height, m_serial);
		if (serial == m_serial)
			return false;
		m_serial = serial;
		return true;
	}

	void Panel::pointer(const uint32_t _id, const int32_t _x, const int32_t _y, const bool _pressed)
	{
		std::lock_guard lock(m_mutex);
		if (m_instance)
			mp_panel_pointer(m_instance, _id, _x, _y, _pressed ? 1 : 0);
	}

	void Panel::pointerLeave(const uint32_t _id)
	{
		std::lock_guard lock(m_mutex);
		if (m_instance)
			mp_panel_pointer_leave(m_instance, _id);
	}
}

#include "panel.h"

#include <algorithm>

namespace mameLib
{
	void Panel::attach(mp_instance* _instance)
	{
		std::lock_guard lock(m_mutex);
		m_instance = _instance;
		m_serial = 0;
		++m_generation;

		// the same for every instance of the machine
		if (m_instance)
		{
			m_fieldNames.clear();
			const auto count = mp_field_count(m_instance);
			for (uint32_t i = 0; i < count; ++i)
			{
				mp_field field{};
				mp_field_info(m_instance, i, &field);
				m_fieldNames.emplace_back(field.name);
			}
		}
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
		// the library only sends frames that show something new; a new buffer needs the current one
		const auto size = static_cast<size_t>(m_width) * m_height * 4;
		const auto last = _rgba.size() == size ? m_serial : 0;
		_rgba.resize(size);
		const auto serial = mp_panel_frame(m_instance, _rgba.data(), m_width, m_height, last);
		if (serial == last)
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

	uint64_t Panel::describeLayout(mp_layout_info& _info, std::vector<mp_layout_element>& _elements, std::vector<mp_layout_item>& _items)
	{
		std::lock_guard lock(m_mutex);
		if (!m_instance)
			return 0;
		mp_layout_describe(m_instance, &_info, nullptr, 0, nullptr, 0);
		_elements.resize(_info.element_count);
		_items.resize(_info.item_count);
		return mp_layout_describe(m_instance, &_info, _elements.data(), _info.element_count, _items.data(), _info.item_count);
	}

	uint64_t Panel::getLayoutStates(const uint64_t _serial, std::vector<mp_layout_item_state>& _states, uint64_t& _layoutSerial)
	{
		std::lock_guard lock(m_mutex);
		if (!m_instance)
			return _serial;
		return mp_layout_states(m_instance, _serial, _states.data(), static_cast<uint32_t>(_states.size()), &_layoutSerial);
	}

	bool Panel::getElementImage(const uint32_t _element, const uint32_t _state, const uint32_t _orientation, const uint32_t _width, const uint32_t _height, std::vector<uint8_t>& _rgba)
	{
		std::lock_guard lock(m_mutex);
		if (!m_instance || !_width || !_height)
			return false;
		_rgba.resize(static_cast<size_t>(_width) * _height * 4);
		return mp_layout_element_image(m_instance, _element, _state, _orientation, _width, _height, _rgba.data()) != 0;
	}

	uint64_t Panel::getScreenImage(const uint32_t _screen, const uint64_t _serial, uint32_t& _width, uint32_t& _height, std::vector<uint8_t>& _rgba)
	{
		std::lock_guard lock(m_mutex);
		if (!m_instance)
			return _serial;
		mp_layout_screen(m_instance, _screen, _serial, &_width, &_height, nullptr, 0);
		_rgba.resize(static_cast<size_t>(_width) * _height * 4);
		return mp_layout_screen(m_instance, _screen, _serial, &_width, &_height, _rgba.data(), _rgba.size());
	}

	void Panel::layoutPointer(const uint32_t _id, const float _x, const float _y, const bool _pressed)
	{
		std::lock_guard lock(m_mutex);
		if (m_instance)
			mp_layout_pointer(m_instance, _id, _x, _y, _pressed ? 1 : 0);
	}

	std::vector<std::string> Panel::getFieldNames()
	{
		std::lock_guard lock(m_mutex);
		return m_fieldNames;
	}

	void Panel::setField(const std::string& _name, const int32_t _value)
	{
		std::lock_guard lock(m_mutex);
		if (!m_instance)
			return;
		const auto it = std::find(m_fieldNames.begin(), m_fieldNames.end(), _name);
		if (it != m_fieldNames.end())
			mp_set_field(m_instance, static_cast<uint32_t>(it - m_fieldNames.begin()), _value);
	}

	uint64_t Panel::getFieldValues(const uint64_t _serial, std::vector<int32_t>& _values)
	{
		std::lock_guard lock(m_mutex);
		if (!m_instance)
			return _serial;
		_values.resize(m_fieldNames.size());
		return mp_field_values(m_instance, _serial, _values.data(), static_cast<uint32_t>(_values.size()));
	}
}

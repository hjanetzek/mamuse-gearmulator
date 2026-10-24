#pragma once

#include <cstdint>
#include <mutex>
#include <vector>

#include "mame_plugin.h"

namespace mameLib
{
	// The machine's front panel as MAME draws it from its layout: artwork, screens, LEDs and clickable controls.
	// Owned by the processor, it outlives the restarts of the machine, e.g. on sample rate or state changes. The device
	// attaches the running instance, the editor draws frames and passes clicks from the message thread
	class Panel
	{
	public:
		// device
		void attach(mp_instance* _instance);
		void detach();

		// editor
		bool getNaturalSize(uint32_t& _width, uint32_t& _height);
		void setSize(uint32_t _width, uint32_t _height);	// 0 stops rendering

		// draws the latest frame as RGBA if there is a new one, returns false otherwise
		bool getFrame(std::vector<uint8_t>& _rgba);
		uint32_t getWidth() const { return m_width; }
		uint32_t getHeight() const { return m_height; }

		// coordinates in pixels of the size passed to setSize
		void pointer(uint32_t _id, int32_t _x, int32_t _y, bool _pressed);
		void pointerLeave(uint32_t _id);

	private:
		std::mutex m_mutex;
		mp_instance* m_instance = nullptr;
		uint32_t m_width = 0;
		uint32_t m_height = 0;
		uint64_t m_serial = 0;
	};
}

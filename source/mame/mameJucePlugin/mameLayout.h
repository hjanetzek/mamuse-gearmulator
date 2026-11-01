#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <tuple>
#include <vector>

#include "juce_events/juce_events.h"

#include "juceRmlUi/rmlElement.h"
#include "juceRmlUi/rmlInstancers.h"

#include "RmlUi/Core/CallbackTexture.h"
#include "RmlUi/Core/Geometry.h"

#include "mame_plugin.h"

namespace mameLib
{
	class Panel;
}

namespace juceRmlUi
{
	class ElemCanvas;
}

namespace mameJucePlugin
{
	// Draws the machine's layout itself, from the description the library gives (docs/layout-loader.md of muse):
	// one textured quad per item, with element images MAME draws at the item's size in pixels and screens at their
	// own size. Where items add or multiply, the region is composited on the CPU as MAME does and drawn as one
	// texture in their place, as RmlUi's renderers only blend with alpha
	class ElemLayout : public juceRmlUi::Element
	{
	public:
		ElemLayout(Rml::CoreInstance& _coreInstance, const Rml::String& _tag, mameLib::Panel& _panel);

		// polls the library, returns true if the layout looks different
		bool update();

		// the topmost clickable item at a position relative to the element's content box, -1 for none
		int clickableItemAt(Rml::Vector2f _pos) const;
		void press(int _item, bool _pressed);

	private:
		// an image as a texture: element images are RGBA as the library draws them, textures need premultiplied alpha
		struct Image
		{
			std::vector<uint8_t> rgba;
			std::vector<uint8_t> premultiplied;
			Rml::Vector2i size;
			Rml::CallbackTexture texture;
		};

		struct Screen
		{
			uint64_t serial = 0;
			Image image;
		};

		// a region with items that add or multiply, composited up to its last such item
		struct Bake
		{
			int x0, y0, x1, y1;		// pixels of the content box
			uint32_t last;			// index of the topmost item in it that adds or multiplies
			Image image;
			bool dirty = true;
		};

		struct Quad
		{
			Rml::Vector2f pos;
			Rml::Vector2f size;
			Rml::ColourbPremultiplied colour;
			Image* image;
			Rml::Geometry geometry;
		};

		// pixel rectangle of an item at the current size, rounded like MAME rounds element items
		struct Rect
		{
			int x, y, w, h;
		};

		void describe();
		void rebuild();
		Rect itemRect(uint32_t _item) const;
		Image* elementImage(uint32_t _item, int _width, int _height);
		void uploadTexture(Image& _image);
		void composite(Bake& _bake);

		void OnRender() override;
		void OnResize() override;

		mameLib::Panel& m_panel;

		uint32_t m_generation = ~0u;
		uint64_t m_layoutSerial = 0;
		mp_layout_info m_info{};
		std::vector<mp_layout_element> m_elements;
		std::vector<mp_layout_item> m_items;
		std::vector<mp_layout_item_state> m_states;
		uint64_t m_statesSerial = 0;
		std::vector<Screen> m_screens;

		Rml::Vector2f m_size{ 0, 0 };	// of the content box the quads were made for
		std::map<std::tuple<uint32_t, uint32_t, uint32_t, int, int>, Image> m_images;	// element, state, orientation, width, height
		std::vector<Bake> m_bakes;
		std::vector<Quad> m_quads;
		bool m_dirty = true;
	};

	// Puts an ElemLayout over the skin's panel canvas and keeps it up to date; clicks press the inputs of the items
	class LayoutView : juce::Timer
	{
	public:
		LayoutView(mameLib::Panel& _panel, juceRmlUi::ElemCanvas* _canvas);
		~LayoutView() override;

		LayoutView(const LayoutView&) = delete;
		LayoutView& operator = (const LayoutView&) = delete;

	private:
		void timerCallback() override;

		mameLib::Panel& m_panel;
		juceRmlUi::CallbackElemInstancer m_instancer;
		ElemLayout* m_layout = nullptr;
		int m_pressedItem = -1;
	};
}

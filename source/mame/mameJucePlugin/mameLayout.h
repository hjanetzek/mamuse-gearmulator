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

#include "mameLib/panel.h"

namespace pluginLib
{
	class Controller;
	class Parameter;
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

		// a position relative to the element's content box in view coordinates, 0..1 across the view
		Rml::Vector2f toView(Rml::Vector2f _pos) const;

		// the index of the item with that id, -1 for none
		int findItem(const std::string& _id) const;
		bool isAnimated(int _item) const;
		uint64_t getLayoutSerial() const { return m_layoutSerial; }
		// the item's size and centre in view coordinates
		Rml::Vector2f getItemSize(int _item) const;
		Rml::Vector2f getItemCentre(int _item) const;

		// moves an item's centre elsewhere than the library puts it, in view coordinates, e.g. a slider's knob that
		// only its script would move. A coordinate that is NaN stays as the library has it. Until the layout changes
		// returns true if it moved
		bool setItemCentre(int _item, Rml::Vector2f _centre);

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
		void applyOverrides();
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
		std::vector<std::string> m_itemIds;
		std::map<int, Rml::Vector2f> m_overrides;	// item centres
		uint64_t m_statesSerial = 0;
		std::vector<Screen> m_screens;

		Rml::Vector2f m_size{ 0, 0 };	// of the content box the quads were made for
		std::map<std::tuple<uint32_t, uint32_t, uint32_t, int, int>, Image> m_images;	// element, state, orientation, width, height
		std::vector<Bake> m_bakes;
		std::vector<Quad> m_quads;
		bool m_dirty = true;
	};

	// Puts an ElemLayout over the skin's panel canvas and keeps it up to date. The layout's knobs and sliders are
	// dragged here and set their parameters, which the controller passes on to the machine. The rest of the mouse
	// goes to MAME in view coordinates: it presses the clickable items and plays the keyboards
	class LayoutView : juce::Timer
	{
	public:
		LayoutView(mameLib::Panel& _panel, pluginLib::Controller& _controller, juceRmlUi::ElemCanvas* _canvas);
		~LayoutView() override;

		LayoutView(const LayoutView&) = delete;
		LayoutView& operator = (const LayoutView&) = delete;

	private:
		// a knob or slider of the layout, dragged here and set as its parameter
		struct Control
		{
			mameLib::Panel::Control control;
			pluginLib::Parameter* parameter = nullptr;
			int knobItem = -1;		// a slider's knob
			bool moveKnob = false;	// which MAME doesn't move, as only the layout's script would
		};

		void timerCallback() override;
		bool updateControls();
		int controlAt(Rml::Vector2f _view) const;
		// a slider's travel, the length its knob's centre moves, along x or y
		float sliderTravel(const Control& _control, bool& _horizontal) const;
		void drag(Rml::Vector2f _view);
		void endDrag();

		mameLib::Panel& m_panel;
		pluginLib::Controller& m_controller;
		std::vector<Control> m_controls;
		uint32_t m_controlsGeneration = ~0u;	// the layout they were made for
		uint64_t m_controlsSerial = 0;
		int m_controlsRetry = 0;
		int m_dragged = -1;
		Rml::Vector2f m_dragStart{ 0, 0 };
		bool m_dragRelative = false;	// a knob, or a slider grabbed by its knob, moves by how far it is dragged
		int m_dragStartValue = 0;
		juceRmlUi::CallbackElemInstancer m_instancer;
		ElemLayout* m_layout = nullptr;
		bool m_pressed = false;
	};
}

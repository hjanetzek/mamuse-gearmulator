#include "mameLayout.h"

#include <algorithm>
#include <cmath>

#include "mameLib/panel.h"

#include "juceRmlUi/juceRmlComponent.h"
#include "juceRmlUi/rmlElemCanvas.h"
#include "juceRmlUi/rmlEventListener.h"
#include "juceRmlUi/rmlHelper.h"

#include "RmlUi/Core/Mesh.h"
#include "RmlUi/Core/MeshUtilities.h"
#include "RmlUi/Core/RenderManager.h"

namespace mameJucePlugin
{
	namespace
	{
		// how often the layout's states are polled, the library captures them at up to 60 Hz
		constexpr int g_updatesPerSecond = 60;

		// as MAME's render_round_nearest
		int roundNearest(const float _f)
		{
			return static_cast<int>(std::floor(_f + 0.5f));
		}

		uint8_t clamp8(const int _v)
		{
			return static_cast<uint8_t>(std::clamp(_v, 0, 255));
		}

		bool blendsOnCpu(const mp_layout_item& _item)
		{
			return _item.blend == MP_LAYOUT_BLEND_MULTIPLY || _item.blend == MP_LAYOUT_BLEND_ADD;
		}

		// one pixel as MAME's software renderer blends it: the image's alpha counts for alpha and add only
		void blend(uint8_t* _dst, const uint8_t* _src, const uint32_t _mode, const mp_layout_item_state& _state)
		{
			const int r = static_cast<int>(static_cast<float>(_src[0]) * _state.r);
			const int g = static_cast<int>(static_cast<float>(_src[1]) * _state.g);
			const int b = static_cast<int>(static_cast<float>(_src[2]) * _state.b);
			const int a = static_cast<int>(static_cast<float>(_src[3]) * _state.a);

			switch (_mode)
			{
			case MP_LAYOUT_BLEND_NONE:
				_dst[0] = clamp8(r);
				_dst[1] = clamp8(g);
				_dst[2] = clamp8(b);
				break;
			case MP_LAYOUT_BLEND_ALPHA:
				if (a)
				{
					_dst[0] = clamp8((r * a + _dst[0] * (256 - a)) >> 8);
					_dst[1] = clamp8((g * a + _dst[1] * (256 - a)) >> 8);
					_dst[2] = clamp8((b * a + _dst[2] * (256 - a)) >> 8);
				}
				break;
			case MP_LAYOUT_BLEND_MULTIPLY:
				_dst[0] = clamp8((r * _dst[0]) >> 8);
				_dst[1] = clamp8((g * _dst[1]) >> 8);
				_dst[2] = clamp8((b * _dst[2]) >> 8);
				break;
			case MP_LAYOUT_BLEND_ADD:
				_dst[0] = clamp8(_dst[0] + ((r * a) >> 8));
				_dst[1] = clamp8(_dst[1] + ((g * a) >> 8));
				_dst[2] = clamp8(_dst[2] + ((b * a) >> 8));
				break;
			default:
				break;
			}
		}
	}

	ElemLayout::ElemLayout(Rml::CoreInstance& _coreInstance, const Rml::String& _tag, mameLib::Panel& _panel)
		: Element(_coreInstance, _tag)
		, m_panel(_panel)
	{
	}

	bool ElemLayout::update()
	{
		bool changed = false;

		// another instance of the machine starts its serials again
		if (m_panel.getGeneration() != m_generation)
		{
			m_generation = m_panel.getGeneration();
			m_layoutSerial = 0;
		}

		// the description, which the library captures once asked for it
		uint64_t layoutSerial = 0;
		auto statesSerial = m_panel.getLayoutStates(m_statesSerial, m_states, layoutSerial);
		if (!m_layoutSerial || layoutSerial != m_layoutSerial)
		{
			describe();
			if (!m_layoutSerial)
				return false;
			statesSerial = m_panel.getLayoutStates(0, m_states, layoutSerial);
			if (layoutSerial != m_layoutSerial)
				return false;
			changed = true;
		}
		if (statesSerial != m_statesSerial)
		{
			m_statesSerial = statesSerial;
			changed = true;
		}
		if (changed)
			m_dirty = true;

		// screens
		std::vector<uint8_t> rgba;
		for (uint32_t i = 0; i < m_screens.size(); ++i)
		{
			auto& screen = m_screens[i];
			uint32_t width = 0, height = 0;
			const auto serial = m_panel.getScreenImage(i, screen.serial, width, height, rgba);
			if (serial == screen.serial || !width || !height)
				continue;
			screen.serial = serial;
			screen.image.rgba = rgba;
			screen.image.size = Rml::Vector2i(static_cast<int>(width), static_cast<int>(height));
			uploadTexture(screen.image);
			for (auto& bake : m_bakes)
				bake.dirty = true;
			changed = true;
		}
		return changed;
	}

	void ElemLayout::describe()
	{
		const auto serial = m_panel.describeLayout(m_info, m_elements, m_items);
		m_layoutSerial = serial;
		m_states.assign(m_items.size(), mp_layout_item_state{});
		m_statesSerial = 0;
		m_screens.clear();
		m_screens.resize(m_info.screen_count);	// sized once: the textures refer to their images
		m_images.clear();
		m_bakes.clear();
		m_quads.clear();
		m_dirty = true;
	}

	ElemLayout::Rect ElemLayout::itemRect(const uint32_t _item) const
	{
		const auto& s = m_states[_item];
		return Rect{
			roundNearest(s.x0 * m_size.x),
			roundNearest(s.y0 * m_size.y),
			roundNearest((s.x1 - s.x0) * m_size.x),
			roundNearest((s.y1 - s.y0) * m_size.y) };
	}

	ElemLayout::Image* ElemLayout::elementImage(const uint32_t _item, const int _width, const int _height)
	{
		const auto& item = m_items[_item];
		const auto state = static_cast<uint32_t>(std::max(m_states[_item].state, 0));
		const auto key = std::make_tuple(item.element, state, item.orientation, _width, _height);
		auto it = m_images.find(key);
		if (it != m_images.end())
			return &it->second;

		Image& image = m_images[key];
		if (!m_panel.getElementImage(item.element, state, item.orientation, static_cast<uint32_t>(_width), static_cast<uint32_t>(_height), image.rgba))
		{
			m_images.erase(key);
			return nullptr;
		}
		image.size = Rml::Vector2i(_width, _height);
		return &image;
	}

	void ElemLayout::uploadTexture(Image& _image)
	{
		// RmlUi's renderers expect premultiplied alpha
		_image.premultiplied.resize(_image.rgba.size());
		for (size_t i = 0; i + 3 < _image.rgba.size(); i += 4)
		{
			const uint32_t a = _image.rgba[i + 3];
			_image.premultiplied[i + 0] = static_cast<uint8_t>((_image.rgba[i + 0] * a + 127) / 255);
			_image.premultiplied[i + 1] = static_cast<uint8_t>((_image.rgba[i + 1] * a + 127) / 255);
			_image.premultiplied[i + 2] = static_cast<uint8_t>((_image.rgba[i + 2] * a + 127) / 255);
			_image.premultiplied[i + 3] = static_cast<uint8_t>(a);
		}

		// same size as before: replace the contents of the existing texture
		if (_image.texture.Update(_image.premultiplied, _image.size))
			return;

		_image.texture.Release();
		if (auto* rm = GetRenderManager())
		{
			_image.texture = rm->MakeCallbackTexture([&_image](const Rml::CallbackTextureInterface& _interface)
			{
				return _interface.GenerateTexture(_image.premultiplied, _image.size);
			});
		}
	}

	void ElemLayout::composite(Bake& _bake)
	{
		const int width = _bake.x1 - _bake.x0;
		const int height = _bake.y1 - _bake.y0;
		auto& rgba = _bake.image.rgba;
		rgba.assign(static_cast<size_t>(width) * height * 4, 0);

		// everything up to the bake's last item that covers its region, as MAME draws it
		for (uint32_t i = 0; i <= _bake.last; ++i)
		{
			const auto& item = m_items[i];
			if (item.flags & MP_LAYOUT_SCROLLED)
				continue;
			const auto r = itemRect(i);
			if (r.w <= 0 || r.h <= 0 || r.x >= _bake.x1 || r.y >= _bake.y1 || r.x + r.w <= _bake.x0 || r.y + r.h <= _bake.y0)
				continue;
			const auto& state = m_states[i];

			if (item.element != MP_LAYOUT_NO_ELEMENT)
			{
				const auto* image = elementImage(i, r.w, r.h);
				if (!image)
					continue;
				for (int y = std::max(r.y, _bake.y0); y < std::min(r.y + r.h, _bake.y1); ++y)
				{
					for (int x = std::max(r.x, _bake.x0); x < std::min(r.x + r.w, _bake.x1); ++x)
					{
						const auto* src = &image->rgba[(static_cast<size_t>(y - r.y) * r.w + (x - r.x)) * 4];
						blend(&rgba[(static_cast<size_t>(y - _bake.y0) * width + (x - _bake.x0)) * 4], src, item.blend, state);
					}
				}
			}
			else if (item.screen < m_screens.size())
			{
				// scaled as MAME's software renderer does: nearest texel from the first pixel's centre, stepping in
				// 16.16 fixed point by the image size over the unrounded bounds
				const auto& image = m_screens[item.screen].image;
				if (image.size.x <= 0 || image.size.y <= 0)
					continue;
				const int dudx = roundNearest(65536.0f * static_cast<float>(image.size.x) / ((state.x1 - state.x0) * m_size.x));
				const int dvdy = roundNearest(65536.0f * static_cast<float>(image.size.y) / ((state.y1 - state.y0) * m_size.y));
				for (int y = std::max(r.y, _bake.y0); y < std::min(r.y + r.h, _bake.y1); ++y)
				{
					const int v = std::min((dvdy / 2 + (y - r.y) * dvdy) >> 16, image.size.y - 1);
					for (int x = std::max(r.x, _bake.x0); x < std::min(r.x + r.w, _bake.x1); ++x)
					{
						const int u = std::min((dudx / 2 + (x - r.x) * dudx) >> 16, image.size.x - 1);
						const auto* src = &image.rgba[(static_cast<size_t>(v) * image.size.x + u) * 4];
						blend(&rgba[(static_cast<size_t>(y - _bake.y0) * width + (x - _bake.x0)) * 4], src, item.blend, state);
					}
				}
			}
		}

		// opaque, as MAME's frame is
		for (size_t i = 3; i < rgba.size(); i += 4)
			rgba[i] = 0xff;
		_bake.image.size = Rml::Vector2i(width, height);
		uploadTexture(_bake.image);
		_bake.dirty = false;
	}

	void ElemLayout::rebuild()
	{
		m_quads.clear();
		m_dirty = false;
		if (m_size.x < 1.0f || m_size.y < 1.0f || m_states.size() != m_items.size())
			return;

		// regions with items that add or multiply, merged where they overlap
		std::vector<Bake> bakes;
		for (uint32_t i = 0; i < m_items.size(); ++i)
		{
			if (!blendsOnCpu(m_items[i]) || (m_items[i].flags & MP_LAYOUT_SCROLLED))
				continue;
			const auto r = itemRect(i);
			if (r.w <= 0 || r.h <= 0)
				continue;
			Bake bake{ r.x, r.y, r.x + r.w, r.y + r.h, i };
			for (auto it = bakes.begin(); it != bakes.end();)
			{
				if (it->x0 < bake.x1 && bake.x0 < it->x1 && it->y0 < bake.y1 && bake.y0 < it->y1)
				{
					bake.x0 = std::min(bake.x0, it->x0);
					bake.y0 = std::min(bake.y0, it->y0);
					bake.x1 = std::max(bake.x1, it->x1);
					bake.y1 = std::max(bake.y1, it->y1);
					it = bakes.erase(it);
				}
				else
				{
					++it;
				}
			}
			bakes.push_back(std::move(bake));
		}
		m_bakes = std::move(bakes);	// not resized from here on: the quads and textures refer to the images
		for (auto& bake : m_bakes)
			composite(bake);

		auto* rm = GetRenderManager();
		if (!rm)
			return;

		auto addQuad = [&](const Rml::Vector2f _pos, const Rml::Vector2f _size, const Rml::ColourbPremultiplied _colour, Image* _image)
		{
			Quad& quad = m_quads.emplace_back();
			quad.pos = _pos;
			quad.size = _size;
			quad.colour = _colour;
			quad.image = _image;
			Rml::Mesh mesh;
			Rml::MeshUtilities::GenerateQuad(mesh, _pos, _size, _colour, Rml::Vector2f(0, 0), Rml::Vector2f(1, 1));
			quad.geometry = rm->MakeGeometry(std::move(mesh));
		};

		for (uint32_t i = 0; i < m_items.size(); ++i)
		{
			const auto& item = m_items[i];
			if (!(item.flags & MP_LAYOUT_SCROLLED) && !blendsOnCpu(item))
			{
				const auto r = itemRect(i);
				const auto& s = m_states[i];
				Image* image = nullptr;
				if (item.element != MP_LAYOUT_NO_ELEMENT)
				{
					if (r.w > 0 && r.h > 0)
					{
						image = elementImage(i, r.w, r.h);
						if (image && !image->texture)
							uploadTexture(*image);
					}
				}
				else if (item.screen < m_screens.size() && m_screens[item.screen].image.size.x > 0)
				{
					image = &m_screens[item.screen].image;
				}

				if (image)
				{
					// the item's colour multiplies the image, premultiplied
					const auto a = std::clamp(s.a, 0.0f, 1.0f);
					const Rml::ColourbPremultiplied colour(
						static_cast<Rml::byte>(std::clamp(s.r * a, 0.0f, 1.0f) * 255.0f + 0.5f),
						static_cast<Rml::byte>(std::clamp(s.g * a, 0.0f, 1.0f) * 255.0f + 0.5f),
						static_cast<Rml::byte>(std::clamp(s.b * a, 0.0f, 1.0f) * 255.0f + 0.5f),
						static_cast<Rml::byte>(a * 255.0f + 0.5f));

					// screens keep their unrounded bounds, as in MAME
					if (item.element == MP_LAYOUT_NO_ELEMENT)
						addQuad(Rml::Vector2f(s.x0 * m_size.x, s.y0 * m_size.y), Rml::Vector2f((s.x1 - s.x0) * m_size.x, (s.y1 - s.y0) * m_size.y), colour, image);
					else
						addQuad(Rml::Vector2f(static_cast<float>(r.x), static_cast<float>(r.y)), Rml::Vector2f(static_cast<float>(r.w), static_cast<float>(r.h)), colour, image);
				}
			}

			// a composited region goes where its last item is
			for (auto& bake : m_bakes)
			{
				if (bake.last == i)
				{
					addQuad(Rml::Vector2f(static_cast<float>(bake.x0), static_cast<float>(bake.y0)),
						Rml::Vector2f(static_cast<float>(bake.x1 - bake.x0), static_cast<float>(bake.y1 - bake.y0)),
						Rml::ColourbPremultiplied(255, 255, 255, 255), &bake.image);
				}
			}
		}
	}

	void ElemLayout::OnRender()
	{
		Element::OnRender();

		const auto size = GetBox().GetSize(Rml::BoxArea::Content);
		if (size != m_size)
		{
			// element images are drawn at the items' sizes in pixels
			m_size = size;
			m_images.clear();
			m_dirty = true;
		}
		if (m_dirty)
		{
			rebuild();
		}
		else
		{
			// a screen in a composited region changed
			for (auto& bake : m_bakes)
			{
				if (bake.dirty)
					composite(bake);
			}
		}

		const auto offset = GetAbsoluteOffset(Rml::BoxArea::Content);
		for (auto& quad : m_quads)
		{
			if (quad.image && quad.image->texture)
				quad.geometry.Render(offset, quad.image->texture);
		}
	}

	void ElemLayout::OnResize()
	{
		Element::OnResize();
		m_dirty = true;
	}

	Rml::Vector2f ElemLayout::toView(const Rml::Vector2f _pos) const
	{
		if (m_size.x < 1.0f || m_size.y < 1.0f)
			return Rml::Vector2f(-1.0f, -1.0f);
		return Rml::Vector2f(_pos.x / m_size.x, _pos.y / m_size.y);
	}

	LayoutView::LayoutView(mameLib::Panel& _panel, juceRmlUi::ElemCanvas* _canvas)
		: m_panel(_panel)
		, m_instancer([&_panel](Rml::CoreInstance& _coreInstance, const Rml::String& _tag)
		{
			return Rml::ElementPtr(new ElemLayout(_coreInstance, _tag, _panel));
		})
	{
		// the canvas stays as the layout's box, showing nothing itself
		_canvas->setRepaintCallback([](std::vector<uint8_t>& _buffer)
		{
			std::fill(_buffer.begin(), _buffer.end(), static_cast<uint8_t>(0));
		});

		auto element = m_instancer.InstanceElement(_canvas->GetCoreInstance(), _canvas, "mamelayout", Rml::XMLAttributes());
		element->SetInstancer(&m_instancer);
		m_layout = dynamic_cast<ElemLayout*>(_canvas->AppendChild(std::move(element)));
		m_layout->SetProperty(Rml::PropertyId::Position, Rml::Style::Position::Absolute);
		m_layout->SetProperty(Rml::PropertyId::Left, Rml::Property(0, Rml::Unit::PX));
		m_layout->SetProperty(Rml::PropertyId::Top, Rml::Property(0, Rml::Unit::PX));
		m_layout->SetProperty(Rml::PropertyId::Width, Rml::Property(100, Rml::Unit::PERCENT));
		m_layout->SetProperty(Rml::PropertyId::Height, Rml::Property(100, Rml::Unit::PERCENT));

		auto pointer = [this](Rml::Event& _event)
		{
			const auto pos = m_layout->toView(juceRmlUi::helper::getMousePos(_event) - m_layout->GetAbsoluteOffset(Rml::BoxArea::Content));
			m_panel.layoutPointer(0, pos.x, pos.y, m_pressed);
		};

		juceRmlUi::EventListener::Add(m_layout, Rml::EventId::Mousedown, [this, pointer](Rml::Event& _event)
		{
			if (juceRmlUi::helper::getMouseButton(_event) != juceRmlUi::MouseButton::Left)
				return;
			m_pressed = true;
			pointer(_event);
		});
		juceRmlUi::EventListener::Add(m_layout, Rml::EventId::Mouseup, [this, pointer](Rml::Event& _event)
		{
			if (juceRmlUi::helper::getMouseButton(_event) != juceRmlUi::MouseButton::Left)
				return;
			m_pressed = false;
			pointer(_event);
		});
		juceRmlUi::EventListener::Add(m_layout, Rml::EventId::Mousemove, pointer);
		juceRmlUi::EventListener::Add(m_layout, Rml::EventId::Mouseout, [this](Rml::Event&)
		{
			m_pressed = false;
			m_panel.pointerLeave(0);
		});

		startTimerHz(g_updatesPerSecond);
	}

	LayoutView::~LayoutView()
	{
		stopTimer();
		if (!m_layout)
			return;
		m_panel.pointerLeave(0);
		if (auto* parent = m_layout->GetParentNode())
			parent->RemoveChild(m_layout);
	}

	void LayoutView::timerCallback()
	{
		if (!m_layout->update())
			return;
		if (auto* comp = juceRmlUi::RmlComponent::fromElement(m_layout))
			comp->enqueueUpdate();
	}
}

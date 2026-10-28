#include "core/Surface.h"
#include <algorithm>
#include <ranges>
#include "core/Application.h"
#include "core/HookStore.h"
#include "input/Scroll.h"
#include "input/Dispatch.h"
#include "input/InputTypes.h"
#include "core/Logger.h"
#include "gfx/TextureRegistry.h"
#include "style/ComputedStyle.h"
#include "utils/ScopedProfiler.h"
#include "yoga/YGNode.h"
#include "yoga/YGNodeLayout.h"

#define mc_scrollStep 16

namespace mocca
{
	constexpr int TextureConsumerGrace = 4;

	static void fireFocusEvent(
		const std::shared_ptr<EventView>& events,
		EventType type,
		FocusReason reason
	)
	{
		if (events == nullptr)
		{
			return;
		}

		FocusEvent ev{.Reason = reason};
		events->Call(type, ev);
	}

	static auto nearestFocusable(detail::Node* node) -> detail::Node*
	{
		for (auto* n = node; n != nullptr; n = n->Parent)
		{
			if (n->Focus != FocusMode::None)
			{
				return n;
			}
		}

		return nullptr;
	}

	static auto nearestCursor(detail::Node* node) -> CursorShape
	{
		for (auto* n = node; n != nullptr; n = n->Parent)
		{
			if (n->Cursor != CursorShape::Default)
			{
				return n->Cursor;
			}
		}

		return CursorShape::Default;
	}

	static void
	collectPreorder(detail::Node* node, std::vector<detail::Node*>& out)
	{
		out.push_back(node);
		for (auto& child : node->Children)
		{
			collectPreorder(child.get(), out);
		}
	}

	Surface::~Surface()
	{
		if (Application::main != nullptr && IsPlatformBacked())
		{
			std::erase(Application::main->_platformSurfaces, this);
		}

		if (_renderer != nullptr && _platform != nullptr)
		{
			_platform->BeginFrame();
			_renderer.reset();
		}

		if (Application::main != nullptr)
		{
			Application::main->Textures().ReleaseConsumerSurface(this);
		}
	}

	void Surface::Update(double dt)
	{
		mc_profileFunction();
		if (_root == nullptr)
		{
			_rootId = detail::nextNodeId;
		}
		std::shared_ptr<EventView> focusedEvents;
		if (auto* focused =
				detail::Node::FindNodeById(_root.get(), _focusedNode))
		{
			focusedEvents = focused->Events;
		}

		auto tree = Element::Render(_desc.Root, _rootId);
		_root = detail::Node::Reconcile(std::move(_root), &tree);

		if (!_root)
		{
			return;
		}

		_rootId = _root->Id;

		if (detail::Node::FindNodeById(_root.get(), _hoveredNode) == nullptr)
		{
			_hoveredNode = 0;
			_updateCursor(nullptr);
		}

		if (_focusedNode != 0)
		{
			auto* focused =
				detail::Node::FindNodeById(_root.get(), _focusedNode);
			if (focused == nullptr)
			{
				fireFocusEvent(
					focusedEvents,
					EventType::Blur,
					FocusReason::Programmatic
				);
				_focusedNode = 0;
			}
			else if (focused->Focus == FocusMode::None)
			{
				ClearFocus();
			}
		}
		if (_capturedNode != 0 &&
			detail::Node::FindNodeById(_root.get(), _capturedNode) == nullptr)
		{
			_capturedNode = 0;
		}

		_root->ComputeStyle(styles::getDefaultStyle(), dt);

		_root->BuildYogaTree();
		{
			mc_profileScope("YGNodeCalculateLayout");
			YGNodeCalculateLayout(
				_root->YogaNode,
				_desc.Width,
				_desc.Height,
				static_cast<YGDirection>(
					static_cast<int>(
						_root->Style
							.Get<styles::StyleProperty::LayoutDirection>()
					) +
					1
				)
			);
		}

		if (IsAutomaticSize())
		{
			_desc.Width = YGNodeLayoutGetWidth(_root->YogaNode);
			_desc.Height = YGNodeLayoutGetHeight(_root->YogaNode);
		}

		detail::reclampScrollOffsets(_root.get());
	}

	void Surface::Tick(double dt)
	{
		mc_profileFunction();
		if (GetState() == SurfaceState::Dead)
		{
			return;
		}

		std::erase_if(
			_children,
			[this](const auto& c) -> auto
			{
				if (c->GetState() != SurfaceState::Dead)
				{
					return false;
				}

				if (_focusSurface == c.get())
				{
					_focusSurface = nullptr;
				}

				if (_captureSurface == c.get())
				{
					_captureSurface = nullptr;
				}

				Surface::AnnounceSubtreeDestroyed(c.get());

				return true;
			}
		);

		if (GetState() == SurfaceState::Alive && IsPlatformBacked())
		{
			auto* ctx = getCtx();
			ctx->_currentSurface = this;

			InputBatch batch;
			GetPlatform()->CollectEvents(batch);
			ProcessInput(batch);

			ctx->_currentSurface = nullptr;
		}

		if (GetState() == SurfaceState::Zombie)
		{
			if (_zombieRemaining > 0)
			{
				_zombieRemaining--;
			}

			if (_zombieRemaining == 0)
			{
				_sweepChildren();
				_state = SurfaceState::Dead;
				return;
			}
		}

		if (IsDirty() || _animating)
		{
			auto* ctx = getCtx();
			ctx->_currentSurface = this;

			_animating = false;

			int settle = 0;
			do
			{
				_dirty = false;
				// only the first pass advances animations; re-entrant passes from
				// the settle loop must not step them again
				Update(settle == 0 ? dt : 0.0);
				ctx->_store->FlushLayoutEffects(this);
			} while (_dirty && ++settle < 16);

			if (_dirty)
			{
				mc_error(
					ErrorCode::InvalidState,
					"layout effects re-dirtied this surface 16 times "
					"without settling (last dirty component: #{})",
					ctx->_store->LastDirty()
				);
				_dirty = false;
			}

			ctx->_currentSurface = nullptr;
			Paint();
		}

		for (auto& i : _children)
		{
			i->Tick(dt);
		}

		Paint();
	}

	void Surface::Paint()
	{
		mc_profileFunction();
		if (!_root)
		{
			return;
		}

		_canvas.Clear();
		_root->Paint(_canvas);

		for (auto& c : _children)
		{
			if (c->IsPlatformBacked())
			{
				continue;
			}

			_canvas.PushClip(
				c->_desc.X,
				c->_desc.Y,
				c->_desc.Width,
				c->_desc.Height
			);
			_canvas.PushTransform(c->_desc.X, c->_desc.Y);
			_canvas.Append(c->_canvas);
			_canvas.PopTransform();
			_canvas.PopClip();
		}

		_dirty = false;
	}

	void Surface::SyncTextureConsumers(gfx::TextureRegistry& registry)
	{
		_textureSeen.clear();
		for (const auto& cmd : _canvas.Commands())
		{
			if (const auto* image = std::get_if<cmds::DrawImageCmd>(&cmd))
			{
				_textureSeen.insert(image->Texture);
			}
		}

		for (auto id : _textureSeen)
		{
			if (registry.IsShared(id))
			{
				continue;
			}

			auto [it, added] = _textureAge.try_emplace(id, 0);
			if (added)
			{
				registry.Acquire(id, this);
			}
			else
			{
				it->second = 0;
			}
		}

		for (auto it = _textureAge.begin(); it != _textureAge.end();)
		{
			if (_textureSeen.contains(it->first))
			{
				++it;
				continue;
			}

			if (++it->second > TextureConsumerGrace)
			{
				registry.Release(it->first, this);
				it = _textureAge.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	auto Surface::FindSurfaceContaining(detail::NodeId id) -> Surface*
	{
		if (detail::Node::FindNodeById(_root.get(), id) != nullptr)
		{
			return this;
		}

		for (auto& c : _children)
		{
			if (auto* found = c->FindSurfaceContaining(id))
			{
				return found;
			}
		}

		return nullptr;
	}

	void Surface::ProcessInput(const InputBatch& batch)
	{
		mc_profileFunction();
		if (!_root)
		{
			return;
		}

		for (const auto& ev : batch.Surface)
		{
			if (ev.EventType == SurfaceEvent::Type::Close)
			{
				RequestClose();
				return;
			}
			if (ev.EventType == SurfaceEvent::Type::Resize)
			{
				_desc.Width = ev.Data1;
				_desc.Height = ev.Data2;
				MarkDirty();
			}
		}

		for (auto ev : batch.Pointer)
		{
			RoutePointer(ev);
		}

		for (auto ev : batch.Keyboard)
		{
			RouteKey(ev);
		}

		for (auto ev : batch.Text)
		{
			RouteText(ev);
		}
	}

	auto hoverChain(detail::Node* node) -> std::vector<detail::NodeId>
	{
		std::vector<detail::NodeId> chain;
		for (auto* n = node; n != nullptr; n = n->Parent)
		{
			chain.push_back(n->Id);
		}
		return chain;
	}

	void Surface::UpdateHover(const PointerEvent& ev)
	{
		if (_root == nullptr)
		{
			return;
		}

		auto* hit = detail::Node::HitTest(_root.get(), ev.X, ev.Y);
		auto next = hoverChain(hit);
		auto prev = hoverChain(
			detail::Node::FindNodeById(_root.get(), _hoveredNode)
		);

		_updateCursor(hit);

		if (next == prev)
		{
			return;
		}

		auto emit = [&](detail::NodeId id, PointerEvent::Type type) -> void
		{
			auto* node = detail::Node::FindNodeById(_root.get(), id);
			if (node == nullptr || node->Events == nullptr)
			{
				return;
			}

			PointerEvent synth = ev;
			synth.EventType = type;

			node->Events->Call(
				type == PointerEvent::Type::Enter ? EventType::PointerEnter
												  : EventType::PointerLeave,
				synth
			);
		};

		for (auto id : prev)
		{
			if (std::ranges::find(next, id) == next.end())
			{
				emit(id, PointerEvent::Type::Leave);
			}
		}

		for (unsigned long& it : std::views::reverse(next))
		{
			if (std::ranges::find(prev, it) == prev.end())
			{
				emit(it, PointerEvent::Type::Enter);
			}
		}

		_hoveredNode = hit != nullptr ? hit->Id : 0;
	}

	// TODO refactor; it now moved to this lol
	void Surface::RoutePointer(PointerEvent ev)
	{
		if (_captureSurface != nullptr)
		{
			if (_captureSurface->GetState() != SurfaceState::Alive)
			{
				_captureSurface = nullptr;
			}
			else
			{
				ev.X -= _captureSurface->_desc.X;
				ev.Y -= _captureSurface->_desc.Y;

				_captureSurface->RoutePointer(ev);

				if (!_captureSurface->IsCapturing())
				{
					_captureSurface = nullptr;
				}
				return;
			}
		}

		for (size_t i = _children.size(); i > 0; --i)
		{
			auto& c = _children[i - 1];

			if (c->GetState() != SurfaceState::Alive || c->IsPlatformBacked())
			{
				continue;
			}

			if (ev.X >= c->_desc.X && ev.X < c->_desc.X + c->_desc.Width &&
				ev.Y >= c->_desc.Y && ev.Y < c->_desc.Y + c->_desc.Height)
			{
				ev.X -= c->_desc.X;
				ev.Y -= c->_desc.Y;

				c->RoutePointer(ev);
				_captureSurface = c->IsCapturing() ? c.get() : nullptr;

				if (ev.EventType == PointerEvent::Type::Down)
				{
					_focusSurface = c.get();
				}

				return;
			}
		}

		if (ev.EventType == PointerEvent::Type::Down && NotifyOutsidePress(ev))
		{
			return;
		}

		if (ev.EventType == PointerEvent::Type::Down)
		{
			_focusSurface = nullptr;
		}

		if (ev.EventType == PointerEvent::Type::Scroll)
		{
			auto* scrollTarget = detail::Node::HitTest(_root.get(), ev.X, ev.Y);
			if (scrollTarget != nullptr)
			{
				auto* scrollable = detail::findScrollableAncestor(scrollTarget);
				if (scrollable != nullptr)
				{
					scrollable->ScrollOffset.X += ev.ScrollX * mc_scrollStep;
					scrollable->ScrollOffset.Y -= ev.ScrollY * mc_scrollStep;

					float cw = YGNodeLayoutGetWidth(scrollable->YogaNode);
					float ch = YGNodeLayoutGetHeight(scrollable->YogaNode);

					float cmaxX = 0.0F;
					float cmaxY = 0.0F;
					detail::contentExtent(scrollable, cmaxX, cmaxY);

					float maxSx = std::max<float>(
						0.0F,
						cmaxX - (scrollable->GetX() + cw)
					);
					float maxSy = std::max<float>(
						0.0F,
						cmaxY - (scrollable->GetY() + ch)
					);

					scrollable->ScrollOffset.X =
						std::clamp(scrollable->ScrollOffset.X, 0.0F, maxSx);
					scrollable->ScrollOffset.Y =
						std::clamp(scrollable->ScrollOffset.Y, 0.0F, maxSy);

					MarkDirty();
				}
			}
			return;
		}

		auto* target = detail::dispatchPointerEvent(
			_root.get(),
			_capturedNode,
			ev,
			[](detail::Node* n, PointerEvent& e) -> void
			{
				if (n->Events == nullptr || e.StopPropagation)
				{
					return;
				}

				EventType type = EventType::PointerDown;
				switch (e.EventType)
				{
				case PointerEvent::Type::Enter:
					type = EventType::PointerEnter;
					break;
				case PointerEvent::Type::Leave:
					type = EventType::PointerLeave;
					break;
				case PointerEvent::Type::Down:
					type = EventType::PointerDown;
					break;
				case PointerEvent::Type::Up:
					type = EventType::PointerUp;
					break;
				case PointerEvent::Type::Move:
					type = EventType::PointerMove;
					break;
				case PointerEvent::Type::Scroll:
					return;
				}

				n->Events->Call(type, e);
			}
		);

		if (ev.EventType == PointerEvent::Type::Down && !ev.PreventDefault)
		{
			auto* focusTarget = nearestFocusable(target);
			if (focusTarget != nullptr)
			{
				FocusNode(focusTarget->Id, FocusReason::Pointer);
			}
			else
			{
				ClearFocus(FocusReason::Pointer);
			}
		}

		if (ev.EventType == PointerEvent::Type::Move ||
			ev.EventType == PointerEvent::Type::Down ||
			ev.EventType == PointerEvent::Type::Up)
		{
			UpdateHover(ev);
		}
	}

	void Surface::RouteKey(KeyEvent& ev)
	{
		if (_focusSurface != nullptr)
		{
			_focusSurface->RouteKey(ev);
			return;
		}

		detail::dispatchKeyTextEvent(
			_root.get(),
			_focusedNode,
			ev,
			[](detail::Node* n, KeyEvent& e) -> void
			{
				if (n->Events == nullptr || e.StopPropagation)
				{
					return;
				}

				EventType type = EventType::KeyDown;
				switch (e.EventType)
				{
				case KeyEvent::Type::Down:
					type = EventType::KeyDown;
					break;
				case KeyEvent::Type::Repeat:
					type = EventType::KeyRepeat;
					break;
				case KeyEvent::Type::Up:
					type = EventType::KeyUp;
					break;
				}

				n->Events->Call(type, e);
			}
		);

		if (ev.Code == KeyCode::Tab && ev.EventType == KeyEvent::Type::Down &&
			!ev.PreventDefault)
		{
			FocusNext(ev.Mods.Shift);
		}
	}

	void Surface::RouteText(TextEvent& ev)
	{
		if (_focusSurface != nullptr)
		{
			_focusSurface->RouteText(ev);
			return;
		}

		detail::dispatchKeyTextEvent(
			_root.get(),
			_focusedNode,
			ev,
			[](detail::Node* n, TextEvent& e) -> void
			{
				if (n->Events == nullptr || e.StopPropagation)
				{
					return;
				}
				n->Events->Call(EventType::TextInput, e);
			}
		);
	}

	auto Surface::NotifyOutsidePress(const PointerEvent& ev) -> bool
	{
		bool consumed = false;

		for (auto& c : std::views::reverse(_children))
		{
			if (c->GetState() != SurfaceState::Alive)
			{
				continue;
			}

			if (c->_onOutsidePress && c->_onOutsidePress(*c, ev))
			{
				consumed = true;
			}

			if (c->NotifyOutsidePress(ev))
			{
				consumed = true;
			}
		}

		return consumed;
	}

	void Surface::FocusNode(detail::NodeId id, FocusReason reason, bool force)
	{
		auto* target = detail::Node::FindNodeById(_root.get(), id);
		if (target == nullptr)
		{
			return;
		}

		if (!force && target->Focus == FocusMode::None)
		{
			return;
		}

		if (_focusedNode == id)
		{
			return;
		}

		auto* old = detail::Node::FindNodeById(_root.get(), _focusedNode);
		std::shared_ptr<EventView> oldEvents = old != nullptr ? old->Events
															  : nullptr;

		_focusedNode = id;

		fireFocusEvent(oldEvents, EventType::Blur, reason);
		fireFocusEvent(target->Events, EventType::Focus, reason);

		MarkDirty();
	}

	void Surface::ClearFocus(FocusReason reason)
	{
		if (_focusedNode == 0)
		{
			return;
		}

		auto* old = detail::Node::FindNodeById(_root.get(), _focusedNode);
		std::shared_ptr<EventView> oldEvents = old != nullptr ? old->Events
															  : nullptr;

		_focusedNode = 0;

		fireFocusEvent(oldEvents, EventType::Blur, reason);

		MarkDirty();
	}

	void Surface::FocusNext(bool backward)
	{
		if (_root == nullptr)
		{
			return;
		}

		std::vector<detail::Node*> all;
		collectPreorder(_root.get(), all);
		if (all.empty())
		{
			return;
		}

		auto count = static_cast<int>(all.size());
		int current = -1;
		for (int i = 0; i < count; ++i)
		{
			if (all[i]->Id == _focusedNode)
			{
				current = i;
				break;
			}
		}

		int start = current >= 0 ? current : (backward ? count : -1);

		for (int step = 1; step <= count; ++step)
		{
			int idx = backward ? (((start - step) % count) + count) % count
							   : (start + step) % count;
			if (all[idx]->Focus == FocusMode::Tab)
			{
				FocusNode(all[idx]->Id, FocusReason::Keyboard);
				return;
			}
		}
	}

	void Surface::CapturePointer(detail::NodeId id)
	{
		_capturedNode = id;
	}

	void Surface::ReleasePointer()
	{
		_capturedNode = 0;
	}

	auto Surface::GetClipboardText() -> std::string
	{
		auto* platform = Application::main->Platform();
		if (platform == nullptr)
		{
			return {};
		}

		return platform->Clipboard()->GetText();
	}

	void Surface::SetClipboardText(std::string_view text)
	{
		auto* platform = Application::main->Platform();
		if (platform == nullptr)
		{
			return;
		}

		platform->Clipboard()->SetText(text);
	}

	void Surface::SetImeCursorRect(Rectangle rect)
	{
		if (_platform != nullptr)
		{
			_platform->SetImeCursorRect(rect);
		}
	}

	void Surface::_updateCursor(detail::Node* node)
	{
		auto shape = nearestCursor(node);
		if (shape == _cursor)
		{
			return;
		}

		_cursor = shape;
		if (_platform != nullptr)
		{
			_platform->SetCursor(shape);
		}
	}

	void Surface::Print(int depth) const
	{
		std::string indent(static_cast<size_t>(depth * 2), ' ');

		mc_info(
			"{}[Surface w={} h={} x={} y={}{}]",
			indent,
			GetDescriptor().Width,
			GetDescriptor().Height,
			GetDescriptor().X,
			GetDescriptor().Y,
			_platform == nullptr ? " (composite)" : ""
		);
		if (_root)
		{
			_root->Print(depth + 1);
		}

		for (const auto& c : _children)
		{
			c->Print(depth + 1);
		}
	}

	void Surface::RequestClose()
	{
		if (_state != SurfaceState::Alive)
		{
			return;
		}

		if (Application::main != nullptr && !Application::main->EmitEvent(
												ApplicationEvent::SurfaceClosed,
												this
											))
		{
			return;
		}

		_state = SurfaceState::Zombie;
		_zombieRemaining = _zombieTimeout;

		for (auto& c : _children)
		{
			c->RequestClose();
		}
	}

	auto Surface::DetachChild(Surface* which) -> std::unique_ptr<Surface>
	{
		std::unique_ptr<Surface> ptr = nullptr;

		std::erase_if(
			_children,
			[&ptr, which](auto& surface) -> auto
			{
				if (surface.get() == which)
				{
					ptr = std::move(surface);
					return true;
				}

				return false;
			}
		);

		return ptr;
	}

	void Surface::AnnounceSubtreeDestroyed(Surface* s)
	{
		if (Application::main == nullptr)
		{
			return;
		}

		for (auto& c : s->_children)
		{
			AnnounceSubtreeDestroyed(c.get());
		}
		Application::main->EmitEvent(ApplicationEvent::SurfaceDestroyed, s);
	}

	void Surface::_sweepChildren()
	{
		for (auto& c : _children)
		{
			if (c->GetState() == SurfaceState::Alive)
			{
				bool cancelled = !Application::main->EmitEvent(
					ApplicationEvent::SurfaceClosed,
					c.get()
				);
				if (cancelled)
				{
					Surface* target = nullptr;
					for (auto* p = this->_desc.Parent; p != nullptr;
						 p = p->_desc.Parent)
					{
						if (p->GetState() == SurfaceState::Alive)
						{
							target = p;
							break;
						}
					}
					Application::main->_pendingReparents
						.emplace_back(this, c.get(), target);
				}
				else
				{
					c->_state = SurfaceState::Zombie;
					c->_zombieRemaining = c->_zombieTimeout;

					for (auto& gc : c->_children)
					{
						gc->RequestClose();
					}
				}
			}
			else if (c->GetState() == SurfaceState::Zombie)
			{
				c->_sweepChildren();
			}
		}
	}
}

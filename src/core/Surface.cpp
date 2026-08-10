#include "Surface.h"
#include <algorithm>
#include <ranges>
#include "Application.h"
#include "InputTypes.h"
#include "Logger.h"
#include "Style.h"
#include "yoga/YGNode.h"

namespace mocca
{
	Surface::~Surface()
	{
		if (Application::main == nullptr)
		{
			// don't know how this could happen but wtv
			return;
		}

		Application::main->EmitEvent(ApplicationEvent::SurfaceDestroyed, this);

		if (IsPlatformBacked())
		{
			std::erase(Application::main->_platformSurfaces, this);
		}
	}

	void Surface::Update(double dt)
	{
		auto tree = Element::Render(_desc.Root, _rootId);
		_root = detail::Node::Reconcile(std::move(_root), &tree);

		if (!_root)
		{
			return;
		}

		_rootId = _root->Id;

		if (_focusedNode != 0 &&
			detail::Node::FindNodeById(_root.get(), _focusedNode) == nullptr)
		{
			_focusedNode = 0;
		}
		if (_capturedNode != 0 &&
			detail::Node::FindNodeById(_root.get(), _capturedNode) == nullptr)
		{
			_capturedNode = 0;
		}

		_root->ComputeStyle(styles::DefaultStyle);

		_root->BuildYogaTree();
		YGNodeCalculateLayout(
			_root->YogaNode,
			_desc.Width,
			_desc.Height,
			(YGDirection)((int)_root->Style.LayoutDirection + 1)
		);

		if (IsAutomaticSize())
		{
			_desc.Width = YGNodeLayoutGetWidth(_root->YogaNode);
			_desc.Height = YGNodeLayoutGetHeight(_root->YogaNode);
		}
	}

	void Surface::Tick(double dt)
	{
		if (GetState() == SurfaceState::Dead)
		{
			return;
		}

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
			if (_zombieTimeout > 0)
			{
				_zombieTimeout--;
			}

			if (_zombieTimeout == 0)
			{
				_state = SurfaceState::Dead;
				return;
			}
		}

		if (IsDirty())
		{
			auto* ctx = getCtx();
			ctx->_currentSurface = this;
			Update(dt);
			ctx->_currentSurface = nullptr;
		}

		for (auto& c : _children)
		{
			c->Tick(dt);
		}

		if (IsDirty())
		{
			Paint();
		}

		if (IsPlatformBacked())
		{
			GetPlatform()->Submit(GetDrawData());
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

				return true;
			}
		);
	}

	void Surface::Paint()
	{
		if (!_root || !_dirty)
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

			_canvas.PushTransform(c->_desc.X, c->_desc.Y);
			_canvas.Append(c->_canvas);
			_canvas.PopTransform();
		}

		_dirty = false;
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

	// TODO refactor; it now moved to this lol
	void Surface::RoutePointer(PointerEvent ev)
	{
		if (_captureSurface != nullptr)
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

		for (auto& c : std::views::reverse(_children))
		{
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
					scrollable->ScrollOffset.X += ev.ScrollX * 16;
					scrollable->ScrollOffset.Y -= ev.ScrollY * 16;

					float cw = YGNodeLayoutGetWidth(scrollable->YogaNode);
					float ch = YGNodeLayoutGetHeight(scrollable->YogaNode);

					float cmaxX = 0.0F;
					float cmaxY = 0.0F;
					uint32_t nc = YGNodeGetChildCount(scrollable->YogaNode);
					for (uint32_t ci = 0; ci < nc; ci++)
					{
						auto* yc = YGNodeGetChild(scrollable->YogaNode, ci);
						cmaxX = std::max<float>(
							cmaxX,
							YGNodeLayoutGetLeft(yc) + YGNodeLayoutGetWidth(yc)
						);
						cmaxY = std::max<float>(
							cmaxY,
							YGNodeLayoutGetTop(yc) + YGNodeLayoutGetHeight(yc)
						);
					}
					float maxSx = std::max<float>(0.0F, cmaxX - cw);
					float maxSy = std::max<float>(0.0F, cmaxY - ch);

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
				if (e.StopPropagation)
				{
					return;
				}
				switch (e.EventType)
				{
				case PointerEvent::Type::Down:
					if (n->Events.OnPointerDown)
					{
						n->Events.OnPointerDown(e);
					}
					break;
				case PointerEvent::Type::Up:
					if (n->Events.OnPointerUp)
					{
						n->Events.OnPointerUp(e);
					}
					break;
				case PointerEvent::Type::Move:
					if (n->Events.OnPointerMove)
					{
						n->Events.OnPointerMove(e);
					}
					break;
				default:
					break;
				}
			}
		);

		if (ev.EventType == PointerEvent::Type::Down && target != nullptr)
		{
			FocusNode(target->Id);
		}
	}

	void Surface::RouteKey(KeyEvent& ev)
	{
		if (_focusSurface != nullptr)
		{
			_focusSurface->RouteKey(ev);
			return;
		}

		detail::dispatchKeyEvent(
			_root.get(),
			_focusedNode,
			ev,
			[](detail::Node* n, KeyEvent& e) -> void
			{
				if (e.StopPropagation)
				{
					return;
				}
				switch (e.EventType)
				{
				case KeyEvent::Type::Down:
					if (n->Events.OnKeyDown)
					{
						n->Events.OnKeyDown(e);
					}
					break;
				case KeyEvent::Type::Up:
					if (n->Events.OnKeyUp)
					{
						n->Events.OnKeyUp(e);
					}
					break;
				case KeyEvent::Type::Repeat:
					if (n->Events.OnKeyRepeat)
					{
						n->Events.OnKeyRepeat(e);
					}
					break;
				}
			}
		);
	}

	void Surface::RouteText(TextEvent& ev)
	{
		if (_focusSurface != nullptr)
		{
			_focusSurface->RouteText(ev);
			return;
		}

		detail::dispatchTextEvent(
			_root.get(),
			_focusedNode,
			ev,
			[](detail::Node* n, TextEvent& e) -> void
			{
				if (e.StopPropagation)
				{
					return;
				}
				if (n->Events.OnTextInput)
				{
					n->Events.OnTextInput(e);
				}
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

	void Surface::FocusNode(detail::NodeId id)
	{
		_focusedNode = id;
	}

	void Surface::ClearFocus()
	{
		_focusedNode = 0;
	}

	void Surface::CapturePointer(detail::NodeId id)
	{
		_capturedNode = id;
	}

	void Surface::ReleasePointer()
	{
		_capturedNode = 0;
	}

	void Surface::Print(int depth) const
	{
		std::string indent(static_cast<size_t>(depth * 2), ' ');

		mc_info(
			"{}[Surface w={} h={}]",
			indent,
			GetDescriptor().Width,
			GetDescriptor().Height
		);
		_root->Print(depth + 1);
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

		for (auto& c : _children)
		{
			c->RequestClose();
		}
	}
}

#include "Surface.h"
#include <algorithm>
#include <ranges>
#include "Application.h"
#include "Detail.h"
#include "InputTypes.h"
#include "Logger.h"
#include "Style.h"
#include "yoga/YGNode.h"

#define mc_scrollStep 16

namespace mocca
{
	Surface::~Surface()
	{
		if (Application::main != nullptr && IsPlatformBacked())
		{
			std::erase(Application::main->_platformSurfaces, this);
		}
	}

	void Surface::Update(double dt)
	{
		if (_root == nullptr)
		{
			_rootId = detail::nextNodeId;
		}
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

		detail::reclampScrollOffsets(_root.get());
	}

	void Surface::Tick(double dt)
	{
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
			if (_zombieTimeout > 0)
			{
				_zombieTimeout--;
			}

			if (_zombieTimeout == 0)
			{
				_sweepChildren();
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

		for (auto& i : _children)
		{
			i->Tick(dt);
		}

		if (IsDirty())
		{
			Paint();
		}

		if (IsPlatformBacked())
		{
			GetPlatform()->Submit(GetDrawData());
		}
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

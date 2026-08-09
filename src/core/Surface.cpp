#include "Surface.h"
#include <algorithm>
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
			(float)_desc.Width,
			(float)_desc.Height,
			(YGDirection)((int)_root->Style.LayoutDirection + 1)
		);
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
			if (_zombieTimer > 0)
			{
				_zombieTimer--;
			}

			if (_zombieTimer == 0)
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
			[](const auto& c) -> auto
			{ return c->GetState() == SurfaceState::Dead; }
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

			_canvas.PushTransform((float)c->_desc.X, (float)c->_desc.Y);
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

	// TODO refactor this thing
	void Surface::ProcessInput(const InputBatch& batch)
	{
		if (!_root)
		{
			return;
		}

		for (const auto& ev : batch.Surface)
		{
			if (ev.EventType == SurfaceEvent::Type::Close &&
				Application::main
					->EmitEvent(ApplicationEvent::SurfaceClosed, this))
			{
				RequestClose();
				return;
			}
			if (ev.EventType == SurfaceEvent::Type::Resize)
			{
				_desc.Width = ev.Width;
				_desc.Height = ev.Height;
				MarkDirty();
			}
		}

		for (auto ev : batch.Pointer)
		{
			if (ev.EventType == PointerEvent::Type::Scroll)
			{
				auto* scrollTarget =
					detail::Node::HitTest(_root.get(), ev.X, ev.Y);
				if (scrollTarget != nullptr)
				{
					auto* scrollable = detail::findScrollableAncestor(
						scrollTarget
					);
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
								YGNodeLayoutGetLeft(yc) +
									YGNodeLayoutGetWidth(yc)
							);
							cmaxY = std::max<float>(
								cmaxY,
								YGNodeLayoutGetTop(yc) +
									YGNodeLayoutGetHeight(yc)
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
				continue;
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

		for (auto ev : batch.Keyboard)
		{
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

		for (auto ev : batch.Text)
		{
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
}

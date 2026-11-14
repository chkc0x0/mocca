#include "core/Surface.h"
#include <algorithm>
#include <optional>
#include <ranges>
#include "a11y/Snapshot.h"
#include "core/Application.h"
#include "core/HookStore.h"
#include "input/Scroll.h"
#include "input/Dispatch.h"
#include "input/InputTypes.h"
#include "core/Logger.h"
#include "gfx/TextureRegistry.h"
#include "style/ComputedStyle.h"
#include "style/StyleProperty.h"
#include "utils/ScopedProfiler.h"
#include "yoga/YGNode.h"
#include "yoga/YGNodeLayout.h"

#define mc_scrollStep 16

namespace mocca
{
	constexpr int TextureConsumerGrace = 4;

	static void FireFocusEvent(
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

	static auto IsPrunedForFocus(const detail::Node& node) -> bool
	{
		for (const auto* n = &node; n != nullptr; n = n->Parent)
		{
			if (n->Style.Get<styles::StyleProperty::Display>() ==
				styles::DisplayType::None)
			{
				return true;
			}

			if (n->A11y != nullptr && n->A11y->Hidden)
			{
				return true;
			}
		}

		return false;
	}

	static auto IsA11yDisabled(const detail::Node& node) -> bool
	{
		return node.A11y != nullptr && node.A11y->Disabled;
	}

	static auto A11yRoleOf(const detail::Node& node) -> aria::Role
	{
		for (const auto* n = &node; n != nullptr; n = n->Parent)
		{
			if (n->A11y != nullptr)
			{
				return n->A11y->Role;
			}
		}

		return aria::Role::None;
	}

	static auto IsActivatable(const detail::Node& node) -> bool
	{
		auto role = A11yRoleOf(node);
		if (aria::isTextInputRole(role))
		{
			return false;
		}

		for (const auto* n = &node; n != nullptr; n = n->Parent)
		{
			if (n->Events != nullptr && n->Events->Has(EventType::Activate))
			{
				return true;
			}
		}

		return aria::isInteractiveRole(role);
	}

	static auto IsFocusable(const detail::Node& node) -> bool
	{
		return node.Focus != FocusMode::None && !IsPrunedForFocus(node) &&
			   !IsA11yDisabled(node);
	}

	static auto IsTabStop(const detail::Node& node) -> bool
	{
		return node.Focus == FocusMode::Tab && !IsPrunedForFocus(node) &&
			   !IsA11yDisabled(node);
	}

	static auto NearestFocusable(detail::Node* node) -> detail::Node*
	{
		for (auto* n = node; n != nullptr; n = n->Parent)
		{
			if (IsFocusable(*n))
			{
				return n;
			}
		}

		return nullptr;
	}

	static auto NearestCursor(detail::Node* node) -> CursorShapeType
	{
		for (auto* n = node; n != nullptr; n = n->Parent)
		{
			auto cur = n->Style.Get<styles::StyleProperty::CursorShape>();
			if (cur != CursorShapeType::Default)
			{
				return cur;
			}
		}

		return CursorShapeType::Default;
	}

	static void
	CollectPreorder(detail::Node* node, std::vector<detail::Node*>& out)
	{
		out.push_back(node);
		for (auto& child : node->Children)
		{
			CollectPreorder(child.get(), out);
		}
	}

	Surface::~Surface()
	{
		_a11y.reset();

		if (Application::main != nullptr && _root != nullptr)
		{
			getCtx()->_store->RemoveComponent(_root.get());
		}

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

	void Surface::UpdateTree(double dt)
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
				FireFocusEvent(
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
		if (_spaceActivateNode != 0 &&
			detail::Node::FindNodeById(_root.get(), _spaceActivateNode) ==
				nullptr)
		{
			_spaceActivateNode = 0;
		}

		_advanceAnimations(dt);
	}

	void Surface::_updateDirtyComponents(
		const std::vector<detail::NodeId>& components,
		double dt
	)
	{
		mc_profileFunction();
		if (_root == nullptr)
		{
			return;
		}

		for (detail::NodeId id : components)
		{
			if (id == _rootId)
			{
				auto tree = mocca::Element::Render(_desc.Root, _rootId);
				_root = detail::Node::Reconcile(std::move(_root), &tree);
				if (_root)
				{
					_rootId = _root->Id;
				}
				continue;
			}

			detail::Node* node = detail::Node::FindNodeById(_root.get(), id);
			if (node == nullptr || !node->IsComponent())
			{
				continue;
			}

			auto& comp = std::get<ComponentElement>(node->Kind);
			std::optional<mocca::Element> produced;
			produced.emplace(
				mocca::Element::Render(comp.Fn, comp.Props, node->Id)
			);

			node->Children = detail::Node::ReconcileChildren(
				std::move(node->Children),
				std::span<const mocca::Element>(&*produced, 1),
				node
			);
		}

		_advanceAnimations(dt);
	}

	void Surface::_advanceAnimations(double dt)
	{
		_root->ComputeStyle(styles::getDefaultStyle(), dt, 0);
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

		_root->UpdateSubtreeBounds();
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

		if (_a11y != nullptr)
		{
			_drainA11yActions();
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

		if (IsDirty() || _animating || !_dirtyComponents.empty() || _relayout)
		{
			Update(dt);
		}

		if (_a11y && _a11yDirty)
		{
			if (_a11y->Active())
			{
				{
					std::scoped_lock lock(_a11yMutex);
					_a11ySnapshot = aria::Snapshot::Build(*this);
				}
				_a11yDirty = false;
			}

			_a11y->UpdateIfActive();
		}

		for (auto& i : _children)
		{
			i->Tick(dt);
		}
	}

	void Surface::Update(double dt)
	{
		auto* ctx = getCtx();
		ctx->_currentSurface = this;

		bool wasAnim = _animating;
		_animating = false;

		int settle = 0;
		do
		{
			bool wasDirty = _dirty;
			_dirty = false;
			std::vector<detail::NodeId> comps = std::move(_dirtyComponents);
			_dirtyComponents.clear();

			if (wasDirty)
			{
				UpdateTree(settle == 0 ? dt : 0.0);
			}
			else if (!comps.empty())
			{
				_updateDirtyComponents(comps, settle == 0 ? dt : 0.0);
			}
			else if (wasAnim || _relayout)
			{
				_advanceAnimations(dt);
				_relayout = false;
			}

			ctx->_store->FlushLayoutEffects(this);
		} while ((_dirty || !_dirtyComponents.empty()) && ++settle < 16);

		if (_dirty || !_dirtyComponents.empty())
		{
			mc_error(
				ErrorCode::InvalidState,
				"layout effects re-dirtied this surface 16 times "
				"without settling (last dirty component: #{})",
				ctx->_store->LastDirty()
			);
			_dirty = false;
			_dirtyComponents.clear();
		}

		ctx->_currentSurface = nullptr;
		_repaint = true;

		MarkA11yDirty();
	}

	void Surface::Paint()
	{
		mc_profileFunction();
		if (!_root || !_repaint)
		{
			return;
		}

		_canvas.Clear();
		_repaint = false;

		auto* ctx = getCtx();
		Surface* previous = ctx->_currentSurface;
		ctx->_currentSurface = this;

		_root->Paint(
			_canvas,
			{
				.X = 0.0F,
				.Y = 0.0F,
				.Width = _desc.Width,
				.Height = _desc.Height,
			}
		);

		for (auto& c : _children)
		{
			if (c->IsPlatformBacked())
			{
				continue;
			}

			_canvas.Save();
			_canvas.ClipRect({
				.X = c->_desc.X,
				.Y = c->_desc.Y,
				.Width = c->_desc.Width,
				.Height = c->_desc.Height,
			});
			_canvas.Translate(c->_desc.X, c->_desc.Y);
			c->Paint();
			_canvas.Append(c->_canvas);
			_canvas.Restore();
		}

		ctx->_currentSurface = previous;
	}

	void Surface::SyncTextureConsumers(gfx::TextureRegistry& registry)
	{
		_textureSeen.clear();
		for (const auto& cmd : _canvas.Commands())
		{
			if (const auto* image = std::get_if<gfx::cmds::DrawImageCmd>(&cmd))
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

	static auto SubtreeContains(const detail::Node* node, detail::NodeId id)
		-> bool
	{
		if (node == nullptr)
		{
			return false;
		}

		if (node->Id == id)
		{
			return true;
		}

		for (const auto& child : node->Children)
		{
			if (SubtreeContains(child.get(), id))
			{
				return true;
			}
		}

		return false;
	}

	auto Surface::FindSurfaceContaining(detail::NodeId id) -> Surface*
	{
		if (SubtreeContains(_root.get(), id))
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
				_relayout = true;
				MarkA11yDirty();
			}
			if (ev.EventType == SurfaceEvent::Type::DPIChange)
			{
				MarkA11yDirty();
				if (_a11y != nullptr)
				{
					_a11y->SetScale(ev.Data1);
				}
			}
			if (ev.EventType == SurfaceEvent::Type::FocusGained)
			{
				if (_a11y != nullptr)
				{
					_a11y->SetWindowFocused(true);
				}
			}
			if (ev.EventType == SurfaceEvent::Type::FocusLost)
			{
				if (_a11y != nullptr)
				{
					_a11y->SetWindowFocused(false);
				}
			}
			if (ev.EventType == SurfaceEvent::Type::RootBoundsChanged)
			{
				if (_a11y != nullptr && _platform != nullptr)
				{
					if (auto bounds = _platform->A11yRootBounds();
						bounds.has_value())
					{
						_a11y->SetRootBounds(*bounds);
					}
				}
			}
		}

		for (size_t i = 0; i < batch.Pointer.size(); i++)
		{
			const auto& ev = batch.Pointer[i];

			// skip consecutive moves cause otherwise we get 70
			if (ev.EventType == PointerEvent::Type::Move)
			{
				if (i + 1 < batch.Pointer.size() &&
					batch.Pointer[i + 1].EventType == PointerEvent::Type::Move)
				{
					continue;
				}
			}

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
		mc_profileFunction();

		std::vector<detail::NodeId> chain;
		for (auto* n = node; n != nullptr; n = n->Parent)
		{
			chain.push_back(n->Id);
		}
		return chain;
	}

	void Surface::UpdateHover(detail::Node* hit, const PointerEvent& ev)
	{
		mc_profileFunction();

		if (_root == nullptr)
		{
			return;
		}

		if (hit == nullptr && ev.EventType != PointerEvent::Type::Leave)
		{
			hit = detail::Node::HitTest(_root.get(), ev.X, ev.Y);
		}

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

		for (auto& it : std::views::reverse(next))
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
		mc_profileFunction();

		if (ev.EventType == PointerEvent::Type::Leave)
		{
			for (auto& c : _children)
			{
				if (c->GetState() == SurfaceState::Alive && !c->IsPlatformBacked())
				{
					c->RoutePointer(ev);
				}
			}

			UpdateHover(nullptr, ev);
			return;
		}

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
					mc_profileScope("RouterPointer.Scroll");
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

					// TODO: if we add scroll dependent stuff, mark it dirty
					RequestRepaint();
					MarkA11yDirty();
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
			auto* focusTarget = NearestFocusable(target);
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
			UpdateHover(_capturedNode == 0 ? target : nullptr, ev);
		}
	}

	void Surface::RouteKey(KeyEvent& ev)
	{
		mc_profileFunction();

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
			return;
		}

		if (ev.Code != KeyCode::Enter && ev.Code != KeyCode::Space)
		{
			return;
		}

		if (ev.PreventDefault)
		{
			if (ev.Code == KeyCode::Space &&
				ev.EventType != KeyEvent::Type::Repeat)
			{
				_spaceActivateNode = 0;
			}
			return;
		}

		auto* target = detail::Node::FindNodeById(_root.get(), _focusedNode);
		if (target == nullptr)
		{
			return;
		}

		if (ev.Code == KeyCode::Enter)
		{
			if (ev.EventType == KeyEvent::Type::Down && IsActivatable(*target))
			{
				ActivateNode(_focusedNode, ActivationSource::Keyboard);
			}
			return;
		}

		switch (ev.EventType)
		{
		case KeyEvent::Type::Down:
			_spaceActivateNode = IsActivatable(*target) ? _focusedNode : 0;
			break;
		case KeyEvent::Type::Up:
		{
			auto id = _spaceActivateNode;
			_spaceActivateNode = 0;
			if (id != 0 && id == _focusedNode)
			{
				ActivateNode(id, ActivationSource::Keyboard);
			}
			break;
		}
		case KeyEvent::Type::Repeat:
			break;
		}
	}

	void Surface::RouteText(TextEvent& ev)
	{
		mc_profileFunction();

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

		if (!force && (target->Focus == FocusMode::None ||
					   IsPrunedForFocus(*target) || IsA11yDisabled(*target)))
		{
			return;
		}

		if (_focusedNode == id)
		{
			return;
		}

		if (reason == FocusReason::Keyboard ||
			reason == FocusReason::AssistiveTechnology)
		{
			detail::scrollRectIntoView(
				target,
				this,
				detail::paintedRect(*target)
			);
		}

		auto* old = detail::Node::FindNodeById(_root.get(), _focusedNode);
		std::shared_ptr<EventView> oldEvents = old != nullptr ? old->Events
															  : nullptr;

		_focusedNode = id;
		_lastFocusReason = reason;
		_spaceActivateNode = 0;

		FireFocusEvent(oldEvents, EventType::Blur, reason);
		FireFocusEvent(target->Events, EventType::Focus, reason);

		MarkDirty();
		MarkA11yDirty();
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
		_spaceActivateNode = 0;

		FireFocusEvent(oldEvents, EventType::Blur, reason);

		MarkDirty();
		MarkA11yDirty();
	}

	auto Surface::GetEffectiveFocus() const -> detail::NodeId
	{
		detail::NodeId focused = _focusedNode;
		const Surface* surface = _focusSurface;

		while (surface != nullptr)
		{
			if (surface->GetState() != SurfaceState::Alive)
			{
				break;
			}

			if (surface->_focusedNode != 0)
			{
				focused = surface->_focusedNode;
			}

			surface = surface->_focusSurface;
		}

		return focused;
	}

	auto Surface::BuildA11ySnapshot() const
		-> std::shared_ptr<const aria::Snapshot>
	{
		return aria::Snapshot::Build(*this);
	}

	void Surface::SetA11yBridge(std::unique_ptr<aria::Bridge> bridge)
	{
		_a11y = std::move(bridge);
		_a11yDirty = true;
	}

	auto Surface::GetA11yBridge() const -> aria::Bridge*
	{
		return _a11y.get();
	}

	void Surface::MarkA11yDirty()
	{
		_a11yDirty = true;
		if (_desc.Parent != nullptr && !IsPlatformBacked())
		{
			_desc.Parent->MarkA11yDirty();
		}
	}

	auto Surface::A11ySnapshot()
		-> std::shared_ptr<const aria::Snapshot>
	{
		std::scoped_lock lock(_a11yMutex);
		return _a11ySnapshot;
	}

	void Surface::QueueA11yAction(aria::ActionRequest req)
	{
		std::scoped_lock lock(_a11yActionsMutex);
		_a11yActions.push_back(std::move(req));
	}

	void Surface::_drainA11yActions()
	{
		std::vector<aria::ActionRequest> actions;
		{
			std::scoped_lock lock(_a11yActionsMutex);
			actions.swap(_a11yActions);
		}

		for (const auto& action : actions)
		{
			DispatchA11yAction(action);
		}
	}

	auto Surface::ActivateNode(detail::NodeId id, ActivationSource source)
		-> bool
	{
		auto* target = detail::Node::FindNodeById(_root.get(), id);
		if (target == nullptr)
		{
			return false;
		}

		ActivateEvent ev{.Source = source};
		bool handled = false;

		for (auto* n = target; n != nullptr; n = n->Parent)
		{
			if (n->Events == nullptr || !n->Events->Has(EventType::Activate))
			{
				continue;
			}

			handled = true;
			n->Events->Call(EventType::Activate, ev);

			if (ev.StopPropagation)
			{
				break;
			}
		}

		return handled;
	}

	void Surface::FocusNodeAnywhere(
		detail::NodeId id,
		FocusReason reason,
		bool force
	)
	{
		auto* owner = FindSurfaceContaining(id);
		if (owner == nullptr)
		{
			return;
		}

		for (auto* surface = this; surface != owner;)
		{
			Surface* next = nullptr;
			for (auto& child : surface->_children)
			{
				if (child.get() == owner ||
					child->FindSurfaceContaining(id) != nullptr)
				{
					next = child.get();
					break;
				}
			}

			if (next == nullptr)
			{
				return;
			}

			surface->_focusSurface = next;
			surface = next;
		}

		owner->_focusSurface = nullptr;
		owner->FocusNode(id, reason, force);
	}

	void Surface::ClearFocusAnywhere(FocusReason reason)
	{
		Surface* target = this;
		for (auto* surface = _focusSurface; surface != nullptr;
			 surface = surface->_focusSurface)
		{
			if (surface->_focusedNode != 0)
			{
				target = surface;
			}
		}

		target->ClearFocus(reason);
	}

	void Surface::DispatchA11yAction(const aria::ActionRequest& request)
	{
		if (_root == nullptr)
		{
			return;
		}

		switch (request.Action)
		{
		case A11yAction::Click:
			if (auto* owner = FindSurfaceContaining(request.Target))
			{
				owner->ActivateNode(
					request.Target,
					ActivationSource::AssistiveTechnology
				);
			}
			return;

		case A11yAction::Focus:
			FocusNodeAnywhere(
				request.Target,
				FocusReason::AssistiveTechnology,
				true
			);
			return;

		case A11yAction::Blur:
			ClearFocusAnywhere(FocusReason::AssistiveTechnology);
			return;

		default:
			break;
		}

		auto* owner = FindSurfaceContaining(request.Target);
		if (owner == nullptr)
		{
			return;
		}

		auto* target =
			detail::Node::FindNodeById(owner->_root.get(), request.Target);
		if (target == nullptr)
		{
			return;
		}

		A11yActionEvent ev{
			.Action = request.Action,
			.Value = request.Value,
			.NumericValue = request.NumericValue,
		};

		for (auto* node = target; node != nullptr && !ev.StopPropagation;
			 node = node->Parent)
		{
			if (node->Events != nullptr &&
				node->Events->Has(EventType::A11yAction))
			{
				node->Events->Call(EventType::A11yAction, ev);
			}
		}
	}

	void Surface::FocusNext(bool backward)
	{
		if (_root == nullptr)
		{
			return;
		}

		std::vector<detail::Node*> all;
		CollectPreorder(_root.get(), all);

		std::vector<detail::Node*> stops;
		stops.reserve(all.size());
		for (auto* node : all)
		{
			if (IsTabStop(*node))
			{
				stops.push_back(node);
			}
		}

		if (stops.empty())
		{
			return;
		}

		auto count = static_cast<int>(stops.size());
		int current = -1;
		for (int i = 0; i < count; ++i)
		{
			if (stops[i]->Id == _focusedNode)
			{
				current = i;
				break;
			}
		}

		int start = current >= 0 ? current : (backward ? 0 : -1);

		for (int step = 1; step <= count; ++step)
		{
			int idx = backward ? (((start - step) % count) + count) % count
							   : (start + step) % count;
			if (idx != current)
			{
				FocusNode(stops[idx]->Id, FocusReason::Keyboard);
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
		auto shape = NearestCursor(node);
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

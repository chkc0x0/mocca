#include "a11y/AccessKitBridge.h"
#include <accesskit.h>
#include <algorithm>
#include <atomic>
#include <utility>

#if defined(_WIN32)
#	define MC_AK_WINDOWS 1
#elif defined(ACCESSKIT_MACOS)
#	define MC_AK_MACOS 1
#elif defined(__linux__) || defined(__DragonFly__) || defined(__FreeBSD__) ||  \
	defined(__NetBSD__) || defined(__OpenBSD__)
#	define MC_AK_UNIX 1
#endif

namespace mocca::aria
{
	auto toAccessKitRole(Role role) -> accesskit_role
	{
		return static_cast<accesskit_role>(role);
	}

	auto toAccessKitRect(const Rectangle& rect) -> accesskit_rect
	{
		return {
			.x0 = rect.X,
			.y0 = rect.Y,
			.x1 = rect.X + rect.Width,
			.y1 = rect.Y + rect.Height,
		};
	}

	auto toAccessKitToggled(Toggled value) -> accesskit_toggled
	{
		switch (value)
		{
		case Toggled::True:
			return ACCESSKIT_TOGGLED_TRUE;
		case Toggled::Mixed:
			return ACCESSKIT_TOGGLED_MIXED;
		default:
			return ACCESSKIT_TOGGLED_FALSE;
		}
	}

	auto toAccessKitLive(Live value) -> accesskit_live
	{
		return value == Live::Assertive ? ACCESSKIT_LIVE_ASSERTIVE
										: ACCESSKIT_LIVE_POLITE;
	}

	auto toAccessKitOrientation(Orientation value) -> accesskit_orientation
	{
		return value == Orientation::Vertical
				   ? ACCESSKIT_ORIENTATION_VERTICAL
				   : ACCESSKIT_ORIENTATION_HORIZONTAL;
	}

	auto toAccessKitInvalid(Invalid value) -> accesskit_invalid
	{
		switch (value)
		{
		case Invalid::Grammar:
			return ACCESSKIT_INVALID_GRAMMAR;
		case Invalid::Spelling:
			return ACCESSKIT_INVALID_SPELLING;
		default:
			return ACCESSKIT_INVALID_TRUE;
		}
	}

	auto toAccessKitCurrent(Current value) -> accesskit_aria_current
	{
		switch (value)
		{
		case Current::Page:
			return ACCESSKIT_ARIA_CURRENT_PAGE;
		case Current::Step:
			return ACCESSKIT_ARIA_CURRENT_STEP;
		case Current::Location:
			return ACCESSKIT_ARIA_CURRENT_LOCATION;
		case Current::Date:
			return ACCESSKIT_ARIA_CURRENT_DATE;
		case Current::Time:
			return ACCESSKIT_ARIA_CURRENT_TIME;
		default:
			return ACCESSKIT_ARIA_CURRENT_TRUE;
		}
	}

	auto toAccessKitHasPopup(HasPopup value) -> accesskit_has_popup
	{
		switch (value)
		{
		case HasPopup::ListBox:
			return ACCESSKIT_HAS_POPUP_LISTBOX;
		case HasPopup::Tree:
			return ACCESSKIT_HAS_POPUP_TREE;
		case HasPopup::Grid:
			return ACCESSKIT_HAS_POPUP_GRID;
		case HasPopup::Dialog:
			return ACCESSKIT_HAS_POPUP_DIALOG;
		default:
			return ACCESSKIT_HAS_POPUP_MENU;
		}
	}

	auto toAccessKitAutoComplete(AutoComplete value) -> accesskit_auto_complete
	{
		switch (value)
		{
		case AutoComplete::List:
			return ACCESSKIT_AUTO_COMPLETE_LIST;
		case AutoComplete::Both:
			return ACCESSKIT_AUTO_COMPLETE_BOTH;
		default:
			return ACCESSKIT_AUTO_COMPLETE_INLINE;
		}
	}

	auto toAccessKitSortDirection(SortDirection value)
		-> accesskit_sort_direction
	{
		switch (value)
		{
		case SortDirection::Descending:
			return ACCESSKIT_SORT_DIRECTION_DESCENDING;
		case SortDirection::Other:
			return ACCESSKIT_SORT_DIRECTION_OTHER;
		default:
			return ACCESSKIT_SORT_DIRECTION_ASCENDING;
		}
	}

	auto toAccessKitAction(accesskit_action action) -> A11yAction
	{
		switch (action)
		{
		case ACCESSKIT_ACTION_CLICK:
			return A11yAction::Click;
		case ACCESSKIT_ACTION_FOCUS:
			return A11yAction::Focus;
		case ACCESSKIT_ACTION_BLUR:
			return A11yAction::Blur;
		case ACCESSKIT_ACTION_EXPAND:
			return A11yAction::Expand;
		case ACCESSKIT_ACTION_COLLAPSE:
			return A11yAction::Collapse;
		case ACCESSKIT_ACTION_INCREMENT:
			return A11yAction::Increment;
		case ACCESSKIT_ACTION_DECREMENT:
			return A11yAction::Decrement;
		case ACCESSKIT_ACTION_SET_VALUE:
			return A11yAction::SetValue;
		default:
			return A11yAction::Custom;
		}
	}

#ifdef MC_A11Y_ENABLED
	auto accessKitRole(Role role) -> uint8_t
	{
		return static_cast<uint8_t>(toAccessKitRole(role));
	}
#endif

	class AccessKitBridge final : public Bridge
	{
	public:
		AccessKitBridge(Provider& provider, const NativeWindow& window)
			: _provider{provider}, _window{window}, _scale{window.Scale}
		{
			_createAdapter();
		}

		~AccessKitBridge() override
		{
			_destroyAdapter();
		}

		void UpdateIfActive() override;
		void SetWindowFocused(bool focused) override;
		void SetRootBounds(const RootBounds& bounds) override;
		void SetScale(float scale) override;

		[[nodiscard]] auto Active() const -> bool override
		{
			return _isActive.load();
		}

	private:
		static auto ActivationCallback(void* userdata) -> accesskit_tree_update*
		{
			auto* self = static_cast<AccessKitBridge*>(userdata);
			self->_isActive.store(true);
			return self->_buildTreeUpdate();
		}

		static void DeactivationCallback(void* userdata)
		{
			static_cast<AccessKitBridge*>(userdata)->_isActive.store(false);
		}

		static void
		ActionCallback(accesskit_action_request* request, void* userdata)
		{
			auto* self = static_cast<AccessKitBridge*>(userdata);
			self->_handleAction(request);
		}

		static auto FactoryCallback(void* userdata) -> accesskit_tree_update*
		{
			return static_cast<AccessKitBridge*>(userdata)->_buildTreeUpdate();
		}

		auto _buildTreeUpdate() -> accesskit_tree_update*;
		auto _rootTransform() const -> accesskit_affine;
		void _handleAction(accesskit_action_request* request);
		auto _buildNode(const Node& node, bool isRoot) -> accesskit_node*;
		void _createAdapter();
		void _destroyAdapter();

		Provider& _provider;
		NativeWindow _window;
		std::atomic<bool> _isActive{false};
		std::atomic<float> _scale{1.0F};

#ifdef MC_AK_WINDOWS
		accesskit_windows_subclassing_adapter* _adapter = nullptr;
#elifdef MC_AK_MACOS
		accesskit_macos_subclassing_adapter* _adapter = nullptr;
#elifdef MC_AK_UNIX
		accesskit_unix_adapter* _adapter = nullptr;
#endif
	};

	auto AccessKitBridge::_rootTransform() const -> accesskit_affine
	{
		return accesskit_affine_scale(_scale.load());
	}

	auto AccessKitBridge::_buildTreeUpdate() -> accesskit_tree_update*
	{
		auto snapshot = _provider.A11ySnapshot();

		size_t capacity = (snapshot != nullptr ? snapshot->Nodes.size() : 0U) +
						  1U;
		accesskit_node_id focus = snapshot != nullptr ? snapshot->Focus : 0;
		auto* update =
			accesskit_tree_update_with_capacity_and_focus(capacity, focus);

		auto* info = accesskit_tree_info_new(0);
		accesskit_tree_info_set_toolkit_name(info, "mocca");
		accesskit_tree_info_set_toolkit_version(info, MC_VERSION);
		accesskit_tree_update_set_tree_info(update, info);

		if (snapshot == nullptr)
		{
			auto* root = accesskit_node_new(ACCESSKIT_ROLE_WINDOW);
			accesskit_node_set_transform(root, _rootTransform());
			accesskit_tree_update_push_node(update, 0, root);
			return update;
		}

		for (const auto& node : snapshot->Nodes)
		{
			accesskit_tree_update_push_node(
				update,
				node.Id,
				_buildNode(node, node.Id == snapshot->Root)
			);
		}

		return update;
	}

	auto AccessKitBridge::_buildNode(const Node& n, bool isRoot)
		-> accesskit_node*
	{
		auto* node = accesskit_node_new(toAccessKitRole(n.Role));
		const auto& label = n.Name.empty() ? n.TextValue : n.Name;

		accesskit_node_set_bounds(node, toAccessKitRect(n.Bounds));
		if (!label.empty())
		{
			accesskit_node_set_label(node, label.c_str());
		}

		if (!n.Children.empty())
		{
			accesskit_node_set_children(
				node,
				n.Children.size(),
				n.Children.data()
			);
		}
		if (!n.LabelledBy.empty())
		{
			accesskit_node_set_labelled_by(
				node,
				n.LabelledBy.size(),
				n.LabelledBy.data()
			);
		}
		if (!n.DescribedBy.empty())
		{
			accesskit_node_set_described_by(
				node,
				n.DescribedBy.size(),
				n.DescribedBy.data()
			);
		}
		if (!n.Controls.empty())
		{
			accesskit_node_set_controls(
				node,
				n.Controls.size(),
				n.Controls.data()
			);
		}
		if (!n.Details.empty())
		{
			accesskit_node_set_details(
				node,
				n.Details.size(),
				n.Details.data()
			);
		}
		if (!n.Owns.empty())
		{
			accesskit_node_set_owns(node, n.Owns.size(), n.Owns.data());
		}
		if (!n.FlowTo.empty())
		{
			accesskit_node_set_flow_to(node, n.FlowTo.size(), n.FlowTo.data());
		}

		if (isRoot)
		{
			accesskit_node_set_transform(node, _rootTransform());
		}

		if (n.Props != nullptr)
		{
			const auto& p = *n.Props;

			if (!p.Description.empty())
			{
				accesskit_node_set_description(node, p.Description.c_str());
			}
			if (!p.Value.empty())
			{
				accesskit_node_set_value(node, p.Value.c_str());
			}
			if (!p.Placeholder.empty())
			{
				accesskit_node_set_placeholder(node, p.Placeholder.c_str());
			}
			if (!p.RoleDescription.empty())
			{
				accesskit_node_set_role_description(
					node,
					p.RoleDescription.c_str()
				);
			}
			if (!p.KeyboardShortcut.empty())
			{
				accesskit_node_set_keyboard_shortcut(
					node,
					p.KeyboardShortcut.c_str()
				);
			}
			if (!p.Language.empty())
			{
				accesskit_node_set_language(node, p.Language.c_str());
			}

			if (p.Disabled)
			{
				accesskit_node_set_disabled(node);
			}
			if (p.Hidden)
			{
				accesskit_node_set_hidden(node);
			}
			if (p.ReadOnly)
			{
				accesskit_node_set_read_only(node);
			}
			if (p.Required)
			{
				accesskit_node_set_required(node);
			}
			if (p.Busy)
			{
				accesskit_node_set_busy(node);
			}
			if (p.Modal)
			{
				accesskit_node_set_modal(node);
			}
			if (p.Multiselectable)
			{
				accesskit_node_set_multiselectable(node);
			}
			if (p.LiveAtomic)
			{
				accesskit_node_set_live_atomic(node);
			}
			if (p.Selected.has_value())
			{
				accesskit_node_set_selected(node, *p.Selected);
			}
			if (p.Expanded.has_value())
			{
				accesskit_node_set_expanded(node, *p.Expanded);
			}
			if (p.Toggled != Toggled::None)
			{
				accesskit_node_set_toggled(node, toAccessKitToggled(p.Toggled));
			}
			if (p.Live != Live::Off)
			{
				accesskit_node_set_live(node, toAccessKitLive(p.Live));
			}
			if (p.Orientation != Orientation::None)
			{
				accesskit_node_set_orientation(
					node,
					toAccessKitOrientation(p.Orientation)
				);
			}
			if (p.Invalid != Invalid::None)
			{
				accesskit_node_set_invalid(node, toAccessKitInvalid(p.Invalid));
			}
			if (p.Current != Current::None)
			{
				accesskit_node_set_aria_current(
					node,
					toAccessKitCurrent(p.Current)
				);
			}
			if (p.HasPopup != HasPopup::None)
			{
				accesskit_node_set_has_popup(
					node,
					toAccessKitHasPopup(p.HasPopup)
				);
			}
			if (p.AutoComplete != AutoComplete::None)
			{
				accesskit_node_set_auto_complete(
					node,
					toAccessKitAutoComplete(p.AutoComplete)
				);
			}
			if (p.SortDirection != SortDirection::None)
			{
				accesskit_node_set_sort_direction(
					node,
					toAccessKitSortDirection(p.SortDirection)
				);
			}

			if (p.NumericValue.has_value())
			{
				accesskit_node_set_numeric_value(node, *p.NumericValue);
			}
			if (p.MinValue.has_value())
			{
				accesskit_node_set_min_numeric_value(node, *p.MinValue);
			}
			if (p.MaxValue.has_value())
			{
				accesskit_node_set_max_numeric_value(node, *p.MaxValue);
			}
			if (p.ValueStep.has_value())
			{
				accesskit_node_set_numeric_value_step(node, *p.ValueStep);
			}
			if (p.ValueJump.has_value())
			{
				accesskit_node_set_numeric_value_jump(node, *p.ValueJump);
			}
			if (p.Level.has_value())
			{
				accesskit_node_set_level(
					node,
					static_cast<size_t>(std::max(0, *p.Level))
				);
			}
			if (p.PositionInSet.has_value())
			{
				accesskit_node_set_position_in_set(
					node,
					static_cast<size_t>(std::max(0, *p.PositionInSet))
				);
			}
			if (p.SizeOfSet.has_value())
			{
				accesskit_node_set_size_of_set(
					node,
					static_cast<size_t>(std::max(0, *p.SizeOfSet))
				);
			}

			if (isInteractiveRole(n.Role) || isTextInputRole(n.Role))
			{
				accesskit_node_add_action(node, ACCESSKIT_ACTION_FOCUS);
			}
			if (isInteractiveRole(n.Role))
			{
				accesskit_node_add_action(node, ACCESSKIT_ACTION_CLICK);
			}
			if (p.Expanded.has_value())
			{
				accesskit_node_add_action(node, ACCESSKIT_ACTION_EXPAND);
				accesskit_node_add_action(node, ACCESSKIT_ACTION_COLLAPSE);
			}
			if (p.NumericValue.has_value())
			{
				accesskit_node_add_action(node, ACCESSKIT_ACTION_INCREMENT);
				accesskit_node_add_action(node, ACCESSKIT_ACTION_DECREMENT);
			}
		}

		return node;
	}

	void AccessKitBridge::_handleAction(accesskit_action_request* request)
	{
		ActionRequest req;
		req.Target = request->target_node;
		req.Action = toAccessKitAction(request->action);

		if (request->data.has_value)
		{
			const auto& data = request->data.value;
			if (data.tag == ACCESSKIT_ACTION_DATA_VALUE &&
				data.value != nullptr)
			{
				req.Value = data.value;
			}
			else if (data.tag == ACCESSKIT_ACTION_DATA_NUMERIC_VALUE)
			{
				req.NumericValue = data.numeric_value;
			}
		}

		_provider.QueueA11yAction(std::move(req));
		accesskit_action_request_free(request);
	}

	void AccessKitBridge::_createAdapter()
	{
#ifdef MC_AK_WINDOWS
		_adapter = accesskit_windows_subclassing_adapter_new(
			static_cast<HWND>(_window.Handle),
			_window.Focused,
			&AccessKitBridge::_activationCallback,
			this,
			&AccessKitBridge::_actionCallback,
			this
		);
#elifdef MC_AK_MACOS
		if (_window.MacWindowClass != nullptr)
		{
			accesskit_macos_add_focus_forwarder_to_window_class(
				_window.MacWindowClass
			);
		}
		_adapter = accesskit_macos_subclassing_adapter_for_window(
			_window.Handle,
			&AccessKitBridge::_activationCallback,
			this,
			&AccessKitBridge::_actionCallback,
			this
		);
#elifdef MC_AK_UNIX
		_adapter = accesskit_unix_adapter_new(
			&AccessKitBridge::ActivationCallback,
			this,
			&AccessKitBridge::ActionCallback,
			this,
			&AccessKitBridge::DeactivationCallback,
			this
		);
		accesskit_unix_adapter_update_window_focus_state(
			_adapter,
			_window.Focused
		);
#endif
	}

	void AccessKitBridge::_destroyAdapter()
	{
#ifdef MC_AK_WINDOWS
		if (_adapter != nullptr)
		{
			accesskit_windows_subclassing_adapter_free(_adapter);
			_adapter = nullptr;
		}
#elifdef MC_AK_MACOS
		if (_adapter != nullptr)
		{
			accesskit_macos_subclassing_adapter_free(_adapter);
			_adapter = nullptr;
		}
#elifdef MC_AK_UNIX
		if (_adapter != nullptr)
		{
			accesskit_unix_adapter_free(_adapter);
			_adapter = nullptr;
		}
#endif
	}

	void AccessKitBridge::UpdateIfActive()
	{
#ifdef MC_AK_WINDOWS
		if (_adapter != nullptr)
		{
			auto* events =
				accesskit_windows_subclassing_adapter_update_if_active(
					_adapter,
					&AccessKitBridge::_factoryCallback,
					this
				);
			if (events != nullptr)
			{
				accesskit_windows_queued_events_raise(events);
			}
		}
#elifdef MC_AK_MACOS
		if (_adapter != nullptr)
		{
			auto* events = accesskit_macos_subclassing_adapter_update_if_active(
				_adapter,
				&AccessKitBridge::_factoryCallback,
				this
			);
			if (events != nullptr)
			{
				accesskit_macos_queued_events_raise(events);
			}
		}
#elifdef MC_AK_UNIX
		if (_adapter != nullptr)
		{
			accesskit_unix_adapter_update_if_active(
				_adapter,
				&AccessKitBridge::FactoryCallback,
				this
			);
		}
#endif
	}

	void AccessKitBridge::SetWindowFocused(bool focused)
	{
#ifdef MC_AK_WINDOWS
		if (_adapter != nullptr)
		{
			auto* events =
				accesskit_windows_subclassing_adapter_update_window_focus_state(
					_adapter,
					focused
				);
			if (events != nullptr)
			{
				accesskit_windows_queued_events_raise(events);
			}
		}
#elifdef MC_AK_MACOS
		if (_adapter != nullptr)
		{
			auto* events =
				accesskit_macos_subclassing_adapter_update_view_focus_state(
					_adapter,
					focused
				);
			if (events != nullptr)
			{
				accesskit_macos_queued_events_raise(events);
			}
		}
#elifdef MC_AK_UNIX
		if (_adapter != nullptr)
		{
			accesskit_unix_adapter_update_window_focus_state(_adapter, focused);
		}
#else
		(void)focused;
#endif
	}

	void AccessKitBridge::SetRootBounds(const RootBounds& bounds)
	{
#ifdef MC_AK_UNIX
		if (_adapter != nullptr)
		{
			accesskit_unix_adapter_set_root_window_bounds(
				_adapter,
				toAccessKitRect(bounds.Outer),
				toAccessKitRect(bounds.Inner)
			);
		}
#else
		(void)bounds;
#endif
	}

	void AccessKitBridge::SetScale(float scale)
	{
		_scale.store(scale);
	}

	auto createAccessKitBridge(Provider& provider, const NativeWindow& window)
		-> std::unique_ptr<Bridge>
	{
#if defined(MC_AK_WINDOWS) || defined(MC_AK_MACOS) || defined(MC_AK_UNIX)
		return std::make_unique<AccessKitBridge>(provider, window);
#else
		(void)provider;
		(void)window;
		return nullptr;
#endif
	}
}

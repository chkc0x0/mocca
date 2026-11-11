#pragma once
#include "core/Identifiers.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace mocca::aria
{
	enum class Role : uint8_t
	{
		None,
		Window,
		Button,
		CheckBox,
		RadioButton,
		Switch,
		Link,
		TextInput,
		MultilineTextInput,
		SearchInput,
		PasswordInput,
		Slider,
		SpinButton,
		ProgressIndicator,
		Meter,
		List,
		ListItem,
		ListBox,
		ListBoxOption,
		Menu,
		MenuItem,
		MenuBar,
		TabList,
		Tab,
		TabPanel,
		Tree,
		TreeItem,
		Grid,
		GridCell,
		Table,
		Row,
		ColumnHeader,
		RowHeader,
		Group,
		Region,
		Article,
		Navigation,
		Main,
		Banner,
		ContentInfo,
		Form,
		Dialog,
		AlertDialog,
		Tooltip,
		Status,
		Alert,
		Image,
		Label,
		Paragraph,
		Heading,
		Separator,
		Scrollbar,
	};

	enum class Toggled : uint8_t
	{
		None,
		False,
		True,
		Mixed,
	};

	enum class Live : uint8_t
	{
		Off,
		Polite,
		Assertive,
	};

	enum class Orientation : uint8_t
	{
		None,
		Horizontal,
		Vertical,
	};

	enum class Invalid : uint8_t
	{
		None,
		True,
		Grammar,
		Spelling,
	};

	enum class Current : uint8_t
	{
		None,
		True,
		Page,
		Step,
		Location,
		Date,
		Time,
	};

	enum class HasPopup : uint8_t
	{
		None,
		Menu,
		ListBox,
		Tree,
		Grid,
		Dialog,
	};

	enum class AutoComplete : uint8_t
	{
		None,
		Inline,
		List,
		Both,
	};

	enum class SortDirection : uint8_t
	{
		None,
		Ascending,
		Descending,
		Other,
	};

	struct Props
	{
		mocca::aria::Role Role = mocca::aria::Role::None;
		uint64_t Id =
			0; // ideally would be replaced with qualified node id (#name) later

		std::string Name; // empty = from content
		std::string Description;
		std::string Value;
		std::string Placeholder;
		std::string RoleDescription;
		std::string KeyboardShortcut;
		std::string Language;

		bool Disabled = false;
		bool Hidden = false; // purges subtree
		bool ReadOnly = false;
		bool Required = false;
		bool Busy = false;
		bool Modal = false;
		bool Multiselectable = false;
		bool LiveAtomic = false;
		std::optional<bool> Selected;
		std::optional<bool> Expanded;
		mocca::aria::Toggled Toggled = mocca::aria::Toggled::None;
		mocca::aria::Live Live = mocca::aria::Live::Off;
		mocca::aria::Orientation Orientation = mocca::aria::Orientation::None;
		mocca::aria::Invalid Invalid = mocca::aria::Invalid::None;
		mocca::aria::Current Current = mocca::aria::Current::None;
		mocca::aria::HasPopup HasPopup = mocca::aria::HasPopup::None;
		mocca::aria::AutoComplete AutoComplete =
			mocca::aria::AutoComplete::None;
		mocca::aria::SortDirection SortDirection =
			mocca::aria::SortDirection::None;

		std::optional<double> NumericValue;
		std::optional<double> MinValue;
		std::optional<double> MaxValue;
		std::optional<double> ValueStep;
		std::optional<double> ValueJump;
		std::optional<int> Level;
		std::optional<int> PositionInSet;
		std::optional<int> SizeOfSet;

		std::vector<uint64_t> LabelledBy;
		std::vector<uint64_t> DescribedBy;
		std::vector<uint64_t> Controls;
		std::vector<uint64_t> Details;
		std::vector<uint64_t> Owns;
		std::vector<uint64_t> FlowTo;

		auto operator==(const Props& other) const -> bool = default;
	};

	struct PropsHash
	{
		auto operator()(const Props& props) const noexcept -> size_t;
	};

	constexpr auto isInteractiveRole(Role role) -> bool
	{
		switch (role)
		{
		case Role::Button:
		case Role::CheckBox:
		case Role::RadioButton:
		case Role::Switch:
		case Role::Link:
		case Role::MenuItem:
		case Role::Tab:
		case Role::TreeItem:
		case Role::ListBoxOption:
		case Role::GridCell:
			return true;
		default:
			return false;
		}
	}

	constexpr auto isTextInputRole(Role role) -> bool
	{
		switch (role)
		{
		case Role::TextInput:
		case Role::MultilineTextInput:
		case Role::SearchInput:
		case Role::PasswordInput:
			return true;
		default:
			return false;
		}
	}

	inline auto id(std::string_view name) -> uint64_t
	{
		return mocca::detail::hashString(name); // FNV-1a, same as ElementKey
	}

	using A11y = std::shared_ptr<const aria::Props>;
}

namespace mocca
{
	auto a11y(aria::Props props) -> aria::A11y;
	auto a11y(aria::Role role, std::string name = {}) -> aria::A11y;
}

namespace mocca::detail
{
	inline auto sameA11y(const aria::A11y& a, const aria::A11y& b) -> bool
	{
		if (a == b)
		{
			return true;
		}
		if (!a || !b)
		{
			return false;
		}
		return *a == *b;
	}
	
	class A11yInterner
	{
	public:
		auto Intern(aria::Props props) -> aria::A11y;
		void Prune();
	private:
		struct Entry
		{
			std::weak_ptr<const aria::Props> Props;
		};

		std::unordered_map<size_t, std::vector<Entry>> _buckets;
	};
}

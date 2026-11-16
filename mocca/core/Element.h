#pragma once
#include "a11y/A11y.h"
#include "core/ElementEvents.h"
#include "core/Part.h"
#include "Identifiers.h"
#include "style/StyleList.h"
#include "utils/ScopedProfiler.h"
#include <any>
#include <cstdint>
#include <functional>
#include <string>
#include <variant>
#include <vector>
#include <ranges>

#define mc_keyNone 0

namespace mocca
{
	enum class FocusMode : uint8_t
	{
		None,
		Program,
		Tab,
	};

	struct Element;
	class NodeRef;

	using ComponentFn = std::function<Element()>;
	using ComponentPropsFn = std::function<Element(const std::any&)>;

	struct BoxElement
	{
		std::vector<Element> Children;
	};

	struct TextElement
	{
		std::string Content;
	};

	struct ComponentElement
	{
		ComponentPropsFn Fn;
		std::any Props;
	};

	struct Element
	{
		std::variant<BoxElement, TextElement, ComponentElement> Node;
		ElementKey Key = mc_keyNone;
		styles::StyleList Style;
		std::shared_ptr<EventView> Events = nullptr;
		NodeRef* Ref = nullptr;
		FocusMode Focus = FocusMode::None;
		aria::A11y A11y = nullptr;
		PartList Parts;

		[[nodiscard]] auto IsBox() const -> bool
		{
			return std::holds_alternative<BoxElement>(Node);
		}
		[[nodiscard]] auto IsText() const -> bool
		{
			return std::holds_alternative<TextElement>(Node);
		}
		[[nodiscard]] auto IsComponent() const -> bool
		{
			return std::holds_alternative<ComponentElement>(Node);
		}

		static auto Render(const ComponentFn& fn, std::uint64_t id) -> Element;
		static auto Render(
			const ComponentPropsFn& fn,
			const std::any& props,
			std::uint64_t id
		) -> Element;
	};

	struct BoxDescriptor
	{
	public:
		aria::A11y A11y = nullptr;
		ElementKey Key = mc_keyNone;
		styles::StyleList Style;
		EventView Events;
		std::vector<Element> Children;
		std::vector<Part> Parts;
		NodeRef* Ref = nullptr;
		FocusMode Focus = FocusMode::None;
	};

	struct TextDescriptor
	{
	public:
		aria::A11y A11y = nullptr;
		ElementKey Key = mc_keyNone;
		styles::StyleList Style;
		EventView Events;
		std::string Content;
		std::vector<Part> Parts;
		NodeRef* Ref = nullptr;
		FocusMode Focus = FocusMode::None;
	};

	struct ComponentDescriptor
	{
	public:
		ElementKey Key = mc_keyNone;
		styles::StyleList Style;
		ComponentFn Fn;
	};

#define mc_baseComponentProps                                                  \
	ElementKey Key = mc_keyNone;                                               \
	styles::StyleList Style;                                                   \
	aria::A11y A11y = nullptr

#define mc_childComponentProps                                                 \
	mc_baseComponentProps;                                                     \
	std::vector<Element> Children

	inline auto box(BoxDescriptor desc = {}) -> Element
	{
		mc_profilePlot("box");
		return Element{
			.Node = BoxElement{.Children = std::move(desc.Children)},
			.Key = desc.Key,
			.Style = desc.Style,
			.Events = EventView::From(std::move(desc.Events)),
			.Ref = desc.Ref,
			.Focus = desc.Focus,
			.A11y = std::move(desc.A11y),
			.Parts = partList(std::move(desc.Parts)),
		};
	}

	inline auto box(std::vector<Element> children, ElementKey key = mc_keyNone)
		-> Element
	{
		mc_profilePlot("box");
		return Element{
			.Node = BoxElement{.Children = std::move(children)},
			.Key = key,
		};
	}

	inline auto text(TextDescriptor desc) -> Element
	{
		mc_profilePlot("text");
		return Element{
			.Node = TextElement{.Content = std::move(desc.Content)},
			.Key = desc.Key,
			.Style = desc.Style,
			.Events = EventView::From(std::move(desc.Events)),
			.Ref = desc.Ref,
			.Focus = desc.Focus,
			.A11y = std::move(desc.A11y),
			.Parts = partList(std::move(desc.Parts)),
		};
	}

	inline auto text(std::string content, ElementKey key = mc_keyNone)
		-> Element
	{
		mc_profilePlot("text");
		return Element{
			.Node = TextElement{.Content = std::move(content)},
			.Key = key,
		};
	}

	inline auto component(ComponentDescriptor& desc) -> Element
	{
		mc_profilePlot("component");
		ComponentPropsFn erased = [desc](const std::any&) -> auto
		{ return desc.Fn(); };
		return Element{
			.Node = ComponentElement{.Fn = erased},
			.Key = desc.Key,
			.Style = desc.Style,
		};
	}

	inline auto component(const ComponentFn& fn, ElementKey key = mc_keyNone)
		-> Element
	{
		mc_profilePlot("component");
		ComponentPropsFn erased = [fn](const std::any&) -> auto
		{ return fn(); };
		return Element{.Node = ComponentElement{.Fn = erased}, .Key = key};
	}

	// do note that desc.Fn will *not* be used
	template <typename Props, typename Fn>
	inline auto
	component(Fn fn, Props props, const ComponentDescriptor& desc = {})
		-> Element
	{
		mc_profilePlot("component");
		ComponentPropsFn erased = [fn](const std::any& boxed) -> auto
		{ return fn(std::any_cast<const Props&>(boxed)); };
		return Element{
			.Node = ComponentElement{.Fn = erased, .Props = props},
			.Key = desc.Key,
			.Style = desc.Style,
		};
	}

	template <std::ranges::range R, typename Fn>
	auto mapElements(R&& items, Fn fn) -> std::vector<Element>
	{
		auto transformed = items | std::ranges::views::transform(fn);
		return std::vector<Element>(transformed.begin(), transformed.end());
	}

	template <typename Fn>
	auto mapElements(size_t count, Fn fn) -> std::vector<Element>
	{
		std::vector<Element> result;
		result.reserve(count);

		for (size_t i = 0; i < count; ++i)
		{
			result.push_back(fn(i));
		}

		return result;
	}

	template <std::ranges::range R, typename Fn>
	auto mapElementsIndexed(R&& items, Fn fn) -> std::vector<Element>
	{
		std::vector<Element> result;
		if constexpr (std::ranges::sized_range<R>)
		{
			result.reserve(std::ranges::size(items));
		}

		size_t i = 0;
		for (auto&& item : items)
		{
			result.push_back(fn(item, i++));
		}

		return result;
	}

	template <typename Arg, typename T>
	concept ElementOrVector =
		std::same_as<std::remove_cvref_t<Arg>, T> ||
		std::same_as<std::remove_cvref_t<Arg>, std::vector<T>>;

	template <ElementOrVector<Element>... Args> auto children(Args&&... args)
	{
		std::vector<Element> out;

		auto get_size = []<typename T>(const T& item) -> std::size_t
		{
			if constexpr (std::same_as<std::remove_cvref_t<T>, Element>)
			{
				return 1;
			}
			else
			{
				return item.size();
			}
		};
		out.reserve((get_size(args) + ... + 0));

		auto append = [&]<typename T>(T&& item)
		{
			if constexpr (std::same_as<std::remove_cvref_t<T>, Element>)
			{
				out.emplace_back(std::forward<T>(item));
			}
			else
			{
				for (auto&& el : item)
				{
					out.emplace_back(std::forward_like<T>(el));
				}
			}
		};
		(append(std::forward<Args>(args)), ...);

		return out;
	}
}

namespace std
{
	template <> struct hash<mocca::ElementKey>
	{
		auto operator()(const mocca::ElementKey& key) const noexcept -> size_t
		{
			return std::hash<uint64_t>{}(static_cast<uint64_t>(key));
		}
	};
}
#pragma once
#include "InputTypes.h"
#include "Style.h"
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
	struct ElementKey
	{
		ElementKey(std::string_view str)
		{
			uint64_t hash = 14695981039346656037ULL;
			for (char c : str)
			{
				hash ^= static_cast<uint8_t>(c);
				hash *= 1099511628211ULL;
			}
			_value = hash;
		}

		ElementKey(uint64_t value) : _value(value) {};
		ElementKey(int value) : _value(static_cast<uint64_t>(value)) {};
		ElementKey() : _value(0) {};
		ElementKey(const char* str) : ElementKey(std::string_view(str)) {};

		operator uint64_t() const
		{
			return _value;
		}

		auto operator<=>(const ElementKey& other) const
		{
			return _value <=> other._value;
		}

		auto operator==(const ElementKey& other) const -> bool
		{
			return _value == other._value;
		}
		auto operator==(int other) const -> bool
		{
			return _value == static_cast<uint64_t>(other);
		}
		auto operator==(std::string_view other) const -> bool
		{
			return ElementKey(other) == *this;
		}

	private:
		uint64_t _value;
	};

	struct Element;

	using ComponentFn = std::function<Element()>;
	using ComponentPropsFn = std::function<Element(const std::any&)>;

	using CleanupFn = std::function<void()>;
	using EffectFn = std::function<CleanupFn()>;

	using PointerCallback = std::function<void(PointerEvent&)>;
	using KeyCallback = std::function<void(KeyEvent&)>;
	using TextCallback = std::function<void(TextEvent&)>;

	struct EventHandlers
	{
		PointerCallback OnPointerDown;
		PointerCallback OnPointerUp;
		PointerCallback OnPointerMove;
		KeyCallback OnKeyDown;
		KeyCallback OnKeyUp;
		KeyCallback OnKeyRepeat;
		TextCallback OnTextInput;
	};

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
		DeclaredStyle Style;
		EventHandlers Events;

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
		ElementKey Key = mc_keyNone;
		DeclaredStyle Style;
		EventHandlers Events;
		std::vector<Element> Children;
	};

	struct TextDescriptor
	{
	public:
		ElementKey Key = mc_keyNone;
		DeclaredStyle Style;
		EventHandlers Events;
		std::string Content;
	};

	struct ComponentDescriptor
	{
	public:
		ElementKey Key = mc_keyNone;
		DeclaredStyle Style;
		ComponentFn Fn;
	};

	inline auto box(BoxDescriptor desc = {}) -> Element
	{
		return Element{
			.Node = BoxElement{.Children = std::move(desc.Children)},
			.Key = desc.Key,
			.Style = desc.Style,
			.Events = std::move(desc.Events)
		};
	}

	inline auto box(std::vector<Element> children, ElementKey key = mc_keyNone)
		-> Element
	{
		return Element{
			.Node = BoxElement{.Children = std::move(children)},
			.Key = key
		};
	}

	inline auto text(TextDescriptor desc) -> Element
	{
		return Element{
			.Node = TextElement{.Content = std::move(desc.Content)},
			.Key = desc.Key,
			.Style = desc.Style,
			.Events = std::move(desc.Events)
		};
	}

	inline auto text(std::string content, ElementKey key = mc_keyNone)
		-> Element
	{
		return Element{
			.Node = TextElement{.Content = std::move(content)},
			.Key = key
		};
	}

	inline auto component(const ComponentDescriptor& desc) -> Element
	{
		ComponentPropsFn erased = [desc](const std::any&) -> auto
		{ return desc.Fn(); };
		return Element{
			.Node = ComponentElement{.Fn = erased},
			.Key = desc.Key,
			.Style = desc.Style
		};
	}

	inline auto component(const ComponentFn& fn, ElementKey key = mc_keyNone)
		-> Element
	{
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
		ComponentPropsFn erased = [fn](const std::any& boxed) -> auto
		{ return fn(std::any_cast<const Props&>(boxed)); };
		return Element{
			.Node = ComponentElement{.Fn = erased, .Props = props},
			.Key = desc.Key,
			.Style = desc.Style
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

		for (int i = 0; i < count; ++i)
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
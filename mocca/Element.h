#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <variant>
#include <vector>

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
		ComponentFn Fn;
	};

	struct Element
	{
		std::variant<BoxElement, TextElement, ComponentElement> Node;
		ElementKey Key = mc_keyNone;

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
	};

	inline auto box(std::vector<Element> children = {},
					ElementKey key = mc_keyNone) -> Element
	{
		return Element{.Node = BoxElement{.Children = std::move(children)},
					   .Key = key};
	}

	inline auto text(std::string content, ElementKey key = mc_keyNone)
		-> Element
	{
		return Element{.Node = TextElement{.Content = std::move(content)},
					   .Key = key};
	}

	inline auto component(const ComponentFn& fn, ElementKey key = mc_keyNone)
		-> Element
	{
		return Element{.Node = ComponentElement{.Fn = fn}, .Key = key};
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
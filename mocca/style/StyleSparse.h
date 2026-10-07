#pragma once
#include <cstdint>
#include <cstring>
#include <functional>
#include <cassert>
#include <span>
#include <vector>
#include "StyleEnums.h"
#include "Length.h"
#include "style/Style.h"
#include "utils/Color.h"

namespace mocca::styles
{
	enum class StyleProperty : char
	{
		None,
#undef mc_styleProperty
#define mc_styleProperty(name, ...) name,
#include "styles.def"
		mc_layoutProperties mc_renderProperties
#undef mc_styleProperty
			StylePropertyCount,
	};

	enum class StylePropertyGroup : char
	{
		Layout,
		Render,
		Text,
	};

	enum class StyleValueType : char
	{
		Keyword,
		Value,
		AuxKeyword,
		Length,
		Color,
	};

	struct StyleEntry
	{
		StyleProperty Prop = StyleProperty::None;
		StyleValueType Type = StyleValueType::Keyword;
		uint8_t Aux = 0;
		uint8_t Pad = 0;
		union
		{
			StyleKeyword Keyword;
			float Float;
			int Int = 0;
			struct Length Length;
			struct Color Color;
		} Payload;
	};

	template <size_t N> struct StyleEntryGroup
	{
		std::array<StyleEntry, N> Entries;
	};

	namespace detail
	{
		template <class T>
		inline constexpr bool IsStyleEntry = std::same_as<T, StyleEntry>;
		template <std::size_t N>
		inline constexpr bool IsStyleEntry<StyleEntryGroup<N>> = true;
	}

	auto entryEqual(const StyleEntry& a, const StyleEntry& b) -> bool;

	class StyleList
	{
	public:
		StyleList() = default;

		template <class... Items>
			requires(
				sizeof...(Items) > 0 && (detail::IsStyleEntry<Items> && ...)
			)
		StyleList(const Items&... items)
		{
			_entries.reserve((ItemCount<Items>() + ...));
			(Add(items), ...);
		}

		void Add(const StyleEntry& e);

		template <std::size_t N> void Add(const StyleEntryGroup<N>& g)
		{
			for (const StyleEntry& e : g.Entries)
			{
				Add(e);
			}
		}

		template <class Item> void AddIf(bool cond, const Item& item)
		{
			if (cond)
			{
				Add(item);
			}
		}

		auto operator+=(const StyleList& rhs) -> StyleList&
		{
			for (const StyleEntry& e : rhs._entries)
			{
				Add(e);
			}
			return *this;
		}

		friend auto operator+(StyleList lhs, const StyleList& rhs) -> StyleList
		{
			lhs += rhs;
			return lhs;
		}

		[[nodiscard]] auto Find(StyleProperty p) const -> const StyleEntry*;

		void Clear()
		{
			_entries.clear();
		}
		[[nodiscard]] auto Size() const -> std::size_t
		{
			return _entries.size();
		}
		[[nodiscard]] auto Empty() const -> bool
		{
			return _entries.empty();
		}
		[[nodiscard]] auto Entries() const -> const std::vector<StyleEntry>&
		{
			return _entries;
		}
		[[nodiscard]] auto begin() const
		{
			return _entries.begin();
		}
		[[nodiscard]] auto end() const
		{
			return _entries.end();
		}

	private:
		template <class Item> static constexpr auto ItemCount() -> std::size_t
		{
			if constexpr (std::same_as<Item, StyleEntry>)
			{
				return 1;
			}
			else
			{
				return std::tuple_size_v<decltype(Item::Entries)>;
			}
		}

		std::vector<StyleEntry> _entries;
	};

	enum class StyleChange : std::uint8_t
	{
		Added,
		Removed,
		Changed,
	};

	class StyleSet
	{
	public:
		StyleSet() = default;
		explicit StyleSet(const StyleList& list)
		{
			Assign(list);
		}

		auto Assign(const StyleList& list) -> bool;

		[[nodiscard]] auto Find(StyleProperty p) const -> const StyleEntry*;

		[[nodiscard]] auto Entries() const -> std::span<const StyleEntry>
		{
			return _entries;
		}
		[[nodiscard]] auto Size() const -> std::size_t
		{
			return _entries.size();
		}
		[[nodiscard]] auto Empty() const -> bool
		{
			return _entries.empty();
		}
		[[nodiscard]] auto begin() const
		{
			return _entries.begin();
		}
		[[nodiscard]] auto end() const
		{
			return _entries.end();
		}

		using StyleChangeFn = std::function<
			void(StyleChange, const StyleEntry*, const StyleEntry*)>;

		static void DiffStyles(
			std::span<const StyleEntry> before,
			std::span<const StyleEntry> after,
			StyleChangeFn&& onChange
		);

	private:
		std::vector<StyleEntry> _entries;
	};

	template <StyleProperty T> struct PropTraits
	{
	};

#undef mc_styleProperty
#define mc_styleProperty(name, type, inherits, initial, declType)              \
	template <> struct PropTraits<StyleProperty::name>                         \
	{                                                                          \
		using Value = type;                                                    \
		static constexpr bool Inherits = inherits;                             \
		static constexpr auto Initial() -> Value                               \
		{                                                                      \
			return initial;                                                    \
		}                                                                      \
		static constexpr StylePropertyGroup Group = mc_styleGroup;             \
	};
#include "styles.def"
#define mc_styleGroup StylePropertyGroup::Layout
	mc_layoutProperties
#undef mc_styleGroup
#define mc_styleGroup StylePropertyGroup::Render
		mc_renderProperties
#undef mc_styleProperty

		namespace detail
	{
		constexpr auto encodeStyleEntry(StyleKeyword k) -> StyleEntry
		{
			return {.Type = StyleValueType::Keyword, .Payload = {.Keyword = k}};
		}

		constexpr auto encodeStyleEntry(Length l) -> StyleEntry
		{
			return {.Type = StyleValueType::Length, .Payload = {.Length = l}};
		}

		constexpr auto encodeStyleEntry(Color c) -> StyleEntry
		{
			return {.Type = StyleValueType::Color, .Payload = {.Color = c}};
		}

		constexpr auto encodeStyleEntry(float f) -> StyleEntry
		{
			return {.Type = StyleValueType::Value, .Payload = {.Float = f}};
		}

		constexpr auto encodeStyleEntry(Alignment a) -> StyleEntry
		{
			return {
				.Type = StyleValueType::AuxKeyword,
				.Aux = static_cast<uint8_t>(a),
			};
		}

		constexpr auto encodeStyleEntry(FlexDirections f) -> StyleEntry
		{
			return {
				.Type = StyleValueType::AuxKeyword,
				.Aux = static_cast<uint8_t>(f),
			};
		}

		constexpr auto encodeStyleEntry(WrappingType w) -> StyleEntry
		{
			return {
				.Type = StyleValueType::AuxKeyword,
				.Aux = static_cast<uint8_t>(w),
			};
		}

		constexpr auto encodeStyleEntry(Justification j) -> StyleEntry
		{
			return {
				.Type = StyleValueType::AuxKeyword,
				.Aux = static_cast<uint8_t>(j),
			};
		}

		constexpr auto encodeStyleEntry(LayoutDirections l) -> StyleEntry
		{
			return {
				.Type = StyleValueType::AuxKeyword,
				.Aux = static_cast<uint8_t>(l),
			};
		}

		constexpr auto encodeStyleEntry(OverflowType o) -> StyleEntry
		{
			return {
				.Type = StyleValueType::AuxKeyword,
				.Aux = static_cast<uint8_t>(o),
			};
		}

		constexpr auto encodeStyleEntry(DisplayType d) -> StyleEntry
		{
			return {
				.Type = StyleValueType::AuxKeyword,
				.Aux = static_cast<uint8_t>(d),
			};
		}

		template <class T>
		constexpr auto encodeStyleEntry(AutoValue<T> k) -> StyleEntry
		{
			return std::visit(
				[](auto&& arg) -> StyleEntry
				{
					using V = std::decay_t<decltype(arg)>;

					if constexpr (std::is_same_v<V, uint8_t>)
					{
						assert(arg == 0);
						return {
							.Type = StyleValueType::AuxKeyword,
							.Aux = 0,
						};
					}
					else
					{
						return encodeStyleEntry<T>(arg);
					}
				},
				k
			);
		}

		template <class T>
		constexpr auto encodeStyleEntry(SizingValue k) -> StyleEntry
		{
			return std::visit(
				[](auto&& arg) -> StyleEntry
				{
					using V = std::decay_t<decltype(arg)>;

					if constexpr (std::is_same_v<V, uint8_t>)
					{
						return {
							.Type = StyleValueType::AuxKeyword,
							.Aux = arg,
						};
					}
					else
					{
						return encodeStyleEntry(arg);
					}
				}
			);
		}

		template <class V>
		constexpr auto edges(V t, V r, V b, V l) -> StyleEntryGroup<4>
		{
			return {
				{
					encodeStyleEntry(t),
					encodeStyleEntry(r),
					encodeStyleEntry(b),
					encodeStyleEntry(l),
				},
			};
		}
	}

#define mc_styleProperty(name, type, ...)                                      \
	constexpr auto name(type val) -> StyleEntry                                \
	{                                                                          \
		return detail::encodeStyleEntry(val);                                  \
	}
	mc_layoutProperties mc_renderProperties
#undef mc_styleProperty

#undef mc_stylePropertyEdge
#define mc_stylePropertyEdge(name, type)                                       \
	constexpr auto name(type a)                                                \
	{                                                                          \
		return detail::edges(a, a, a, a);                                      \
	}                                                                          \
	constexpr auto name(type v, type h)                                        \
	{                                                                          \
		return detail::edges(v, h, v, h);                                      \
	}                                                                          \
	constexpr auto name(type t, type b, type h)                                \
	{                                                                          \
		return detail::edges(t, h, b, h);                                      \
	}                                                                          \
	constexpr auto name(type t, type r, type b, type l)                        \
	{                                                                          \
		return detail::edges(t, r, b, l);                                      \
	}

		mc_propertyEdges
#undef mc_stylePropertyEdge
}
#pragma once
#include <cstdint>
#include <cstring>
#include <functional>
#include <cassert>
#include <span>
#include <vector>
#include "StyleEnums.h"
#include "Length.h"
#include "core/Logger.h"
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
		LayoutSizing,
		LayoutSpacing,
		LayoutFlex,
		LayoutPosition,
		Render,
		Inherited,
	};

	enum class StyleValueType : char
	{
		Keyword,
		Value,
		AuxKeyword,
		Length,
		Color,
	};

	struct StyleEntry;

	template <typename T> struct StyleEntryReader
	{
		static auto Read(const StyleEntry& /*unused*/) -> T
		{
			static_assert(false, "no StyleEntryReader for this type");
			return T{};
		}
	};

	template <typename T> auto getStyleEntryValue(const StyleEntry& s) -> T
	{
		return StyleEntryReader<T>::Read(s);
	}

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
			uint8_t Enum;
			struct Length Length;
			struct Color Color;
		} Payload;

		[[nodiscard]] auto IsValue() const -> bool
		{
			return Type != StyleValueType::Keyword;
		};

		template <typename Prop>
		auto Resolve(const Prop::Value& parentValue, const Prop& prop) const
			-> Prop::Value
		{
			if (IsValue())
			{
				return getStyleEntryValue<typename Prop::Value>(*this);
			}

			if (Type == StyleValueType::Keyword &&
				Payload.Keyword == StyleKeyword::Inherit)
			{
				return parentValue;
			}

			if (Type == StyleValueType::Keyword &&
				Payload.Keyword == StyleKeyword::Initial)
			{
				return Prop::Initial();
			}

			return Prop::Inherits ? parentValue : Prop::Initial();
		}
	};

	template <> struct StyleEntryReader<Length>
	{
		static auto Read(const StyleEntry& s) -> Length
		{
			return s.Payload.Length;
		}
	};

	template <> struct StyleEntryReader<Color>
	{
		static auto Read(const StyleEntry& s) -> Color
		{
			return s.Payload.Color;
		}
	};

	template <> struct StyleEntryReader<float>
	{
		static auto Read(const StyleEntry& s) -> float
		{
			return s.Payload.Float;
		}
	};

	template <> struct StyleEntryReader<int>
	{
		static auto Read(const StyleEntry& s) -> int
		{
			return s.Payload.Int;
		}
	};

	template <typename T>
		requires std::is_enum_v<T>
	struct StyleEntryReader<T>
	{
		static auto Read(const StyleEntry& s) -> T
		{
			return static_cast<T>(s.Payload.Enum);
		}
	};

	template <typename T> struct StyleEntryReader<std::variant<T, uint8_t>>
	{
		static auto Read(const StyleEntry& s) -> std::variant<T, uint8_t>
		{
			if (s.Type == StyleValueType::AuxKeyword && s.Aux == Auto)
			{
				return Auto;
			}
			return StyleEntryReader<T>::Read(s);
		}
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

	class ComputedStyle
	{
	public:
#define mc_styleProperty(name, type, inh, initial, ...) type name = initial;
		struct LayoutSizingProps
		{
			mc_layoutSizingProps
		} LayoutSizing;

		struct LayoutSpacingProps
		{
			mc_layoutSpacingProps
		} LayoutSpacing;

		struct LayoutFlexProps
		{
			mc_layoutFlexProps
		} LayoutFlex;

		struct LayoutPositionProps
		{
			mc_layoutPosProps
		} LayoutPosition;

		struct RenderProps
		{
			mc_renderPropertiesReset
		} Render;

		// maybe switch to per category later
		// if it's a problem
		struct InheritedProps
		{
			mc_layoutPropertiesInh mc_renderPropertiesInh
		} Inherited;
#undef mc_styleProperty

		void Apply(const StyleEntry& e, ComputedStyle& parent);
		static auto Cascade(const StyleSet& s, ComputedStyle& parent)
			-> ComputedStyle;

		template <StyleProperty P>
		[[nodiscard]] auto Get() const -> const PropTraits<P>::Value&;

	private:
		template <StyleProperty P> auto _ref() -> PropTraits<P>::Value&;
	};

#undef mc_styleProperty
#define mc_cat_(a, b) a##b
#define mc_cat(a, b) mc_cat_(a, b)
#define mc_styleProperty(name, type, inherits, initial, ...)                   \
	template <> struct PropTraits<StyleProperty::name>                         \
	{                                                                          \
		using Value = type;                                                    \
		static constexpr bool Inherits = inherits;                             \
		static constexpr auto Initial() -> Value                               \
		{                                                                      \
			return initial;                                                    \
		}                                                                      \
		static constexpr StylePropertyGroup Group =                            \
			StylePropertyGroup::mc_styleGroup;                                 \
		static constexpr auto Handle = &ComputedStyle::mc_styleGroup;          \
		static constexpr auto Field =                                          \
			&ComputedStyle::mc_cat(mc_styleGroup, Props)::name;                \
	};
#include "styles.def"
#define mc_styleGroup LayoutSizing
	mc_layoutSizingProps
#undef mc_styleGroup
#define mc_styleGroup LayoutSpacing
		mc_layoutSpacingProps
#undef mc_styleGroup
#define mc_styleGroup LayoutFlex
			mc_layoutFlexProps
#undef mc_styleGroup
#define mc_styleGroup LayoutPosition
				mc_layoutPosProps
#undef mc_styleGroup
#define mc_styleGroup Render
					mc_renderPropertiesReset
#undef mc_styleGroup
#define mc_styleGroup Inherited
						mc_inheritProperties
#undef mc_styleGroup
#undef mc_styleProperty
#undef mc_cat_
#undef mc_cat

		namespace detail
	{
		constexpr auto encodeStyleEntry(StyleProperty prop, StyleKeyword k)
			-> StyleEntry
		{
			return {
				.Prop = prop,
				.Type = StyleValueType::Keyword,
				.Payload = {.Keyword = k}
			};
		}

		constexpr auto encodeStyleEntry(StyleProperty prop, Length l)
			-> StyleEntry
		{
			return {
				.Prop = prop,
				.Type = StyleValueType::Length,
				.Payload = {.Length = l},
			};
		}

		constexpr auto encodeStyleEntry(StyleProperty prop, Color c)
			-> StyleEntry
		{
			return {
				.Prop = prop,
				.Type = StyleValueType::Color,
				.Payload = {.Color = c},
			};
		}

		constexpr auto encodeStyleEntry(StyleProperty prop, float f)
			-> StyleEntry
		{
			return {
				.Prop = prop,
				.Type = StyleValueType::Value,
				.Payload = {.Float = f},
			};
		}

		constexpr auto encodeStyleEntry(StyleProperty prop, Alignment a)
			-> StyleEntry
		{
			return {
				.Prop = prop,
				.Type = StyleValueType::AuxKeyword,
				.Payload = {.Enum = static_cast<uint8_t>(a)},
			};
		}

		constexpr auto encodeStyleEntry(StyleProperty prop, FlexDirections f)
			-> StyleEntry
		{
			return {
				.Prop = prop,
				.Type = StyleValueType::AuxKeyword,
				.Payload = {.Enum = static_cast<uint8_t>(f)},
			};
		}

		constexpr auto encodeStyleEntry(StyleProperty prop, WrappingType w)
			-> StyleEntry
		{
			return {
				.Prop = prop,
				.Type = StyleValueType::AuxKeyword,
				.Payload = {.Enum = static_cast<uint8_t>(w)},
			};
		}

		constexpr auto encodeStyleEntry(StyleProperty prop, Justification j)
			-> StyleEntry
		{
			return {
				.Prop = prop,
				.Type = StyleValueType::AuxKeyword,
				.Payload = {.Enum = static_cast<uint8_t>(j)},
			};
		}

		constexpr auto encodeStyleEntry(StyleProperty prop, LayoutDirections l)
			-> StyleEntry
		{
			return {
				.Prop = prop,
				.Type = StyleValueType::AuxKeyword,
				.Payload = {.Enum = static_cast<uint8_t>(l)},
			};
		}

		constexpr auto encodeStyleEntry(StyleProperty prop, OverflowType o)
			-> StyleEntry
		{
			return {
				.Prop = prop,
				.Type = StyleValueType::AuxKeyword,
				.Payload = {.Enum = static_cast<uint8_t>(o)},
			};
		}

		constexpr auto encodeStyleEntry(StyleProperty prop, DisplayType d)
			-> StyleEntry
		{
			return {
				.Prop = prop,
				.Type = StyleValueType::AuxKeyword,
				.Payload = {.Enum = static_cast<uint8_t>(d)},
			};
		}

		template <class T>
		constexpr auto encodeStyleEntry(StyleProperty prop, AutoValue<T> k)
			-> StyleEntry
		{
			return std::visit(
				[prop](auto&& arg) -> StyleEntry
				{
					using V = std::decay_t<decltype(arg)>;

					if constexpr (std::is_same_v<V, uint8_t>)
					{
						assert(arg == 0);
						return {
							.Prop = prop,
							.Type = StyleValueType::AuxKeyword,
							.Aux = 0,
						};
					}
					else
					{
						return encodeStyleEntry(prop, arg);
					}
				},
				k
			);
		}

		constexpr auto encodeStyleEntry(StyleProperty prop, SizingValue k)
			-> StyleEntry
		{
			return std::visit(
				[prop](auto&& arg) -> StyleEntry
				{
					using V = std::decay_t<decltype(arg)>;

					if constexpr (std::is_same_v<V, uint8_t>)
					{
						return {
							.Prop = prop,
							.Type = StyleValueType::AuxKeyword,
							.Aux = arg,
						};
					}
					else
					{
						return encodeStyleEntry(prop, arg);
					}
				},
				k
			);
		}

		template <class V>
		constexpr auto edges(StyleProperty prop, V t, V r, V b, V l)
			-> StyleEntryGroup<4>
		{
			return {
				{
					encodeStyleEntry(prop, t),
					encodeStyleEntry((StyleProperty)((uint8_t)prop + 1), r),
					encodeStyleEntry((StyleProperty)((uint8_t)prop + 2), b),
					encodeStyleEntry((StyleProperty)((uint8_t)prop + 3), l),
				},
			};
		}

		template <class T> auto encodeStyleEntry(T)->StyleEntry = delete;
	}

#define mc_styleProperty(name, type, ...)                                      \
	constexpr auto name(type val) -> StyleEntry                                \
	{                                                                          \
		return detail::encodeStyleEntry(StyleProperty::name, val);             \
	}
	mc_layoutProperties mc_renderProperties
#undef mc_styleProperty

#undef mc_stylePropertyEdge
#define mc_stylePropertyEdge(name, type)                                       \
	constexpr auto name(type a)                                                \
	{                                                                          \
		return detail::edges(StyleProperty::name##Top, a, a, a, a);            \
	}                                                                          \
	constexpr auto name(type v, type h)                                        \
	{                                                                          \
		return detail::edges(StyleProperty::name##Top, v, h, v, h);            \
	}                                                                          \
	constexpr auto name(type t, type b, type h)                                \
	{                                                                          \
		return detail::edges(StyleProperty::name##Top, t, h, b, h);            \
	}                                                                          \
	constexpr auto name(type t, type r, type b, type l)                        \
	{                                                                          \
		return detail::edges(StyleProperty::name##Top, t, l, b, r);            \
	}

		mc_propertyEdges
#undef mc_stylePropertyEdge

		// i somehow got the order wrong lol
		// but i already applied it everywhere so i wont change it
		// ^ (it was trbl instead of tlbr)

		template <StyleProperty P>
		auto ComputedStyle::Get() const -> const PropTraits<P>::Value&
	{
		using L = PropTraits<P>;
		return (this->*L::Handle).*L::Field;
	}

	template <StyleProperty P>
	auto ComputedStyle::_ref() -> PropTraits<P>::Value&
	{
		using L = PropTraits<P>;
		return (this->*L::Handle).*L::Field;
	}

	auto getDefaultStyle() -> ComputedStyle&;
}
#pragma once
#include "Logger.h"
#include "Math.h"
#include "styles.def"
#include "yoga/YGConfig.h"
#include <variant>

namespace mocca
{
	namespace detail
	{
		struct Node;
	}

	enum class StyleKeyword : char
	{
		Unset,
		Inherit,
		Initial
	};

	enum class Alignment : char
	{
		FlexStart,
		Center,
		FlexEnd,
		Stretch,
		Baseline,
		SpaceBetween,
		SpaceAround,
		SpaceEvenly,
		Start,
		End
	};

	enum class DisplayType : char
	{
		Flex,
		None,
		Contents,
		Grid
	};

	enum class FlexDirections : char
	{
		Column,
		ColumnReverse,
		Row,
		RowReverse
	};

	enum class WrappingType : char
	{
		None,
		Wrap,
		WrapReverse
	};

	enum class Justification : char
	{
		FlexStart,
		Center,
		FlexEnd,
		SpaceBetween,
		SpaceAround,
		SpaceEvenly,
		Stretch,
		Start,
		End
	};

	enum class LayoutDirections : char
	{
		LTR,
		RTL
	};

	enum class OverflowType : char
	{
		Visible,
		Hidden,
		Scroll
	};

	template <typename T> struct StyleValue
	{
	public:
		StyleValue(T value) : _data(value) {};
		StyleValue(StyleKeyword value) : _data(value) {};
		StyleValue() : _data(StyleKeyword::Unset) {};

		[[nodiscard]] auto GetValue() const -> T
		{
			if (!IsValue())
			{
				mc_error(ErrorCode::InvalidState, "no value stored here!");
				return {};
			}

			return std::get<T>(_data);
		}

		[[nodiscard]] auto IsValue() const -> bool
		{
			return std::holds_alternative<T>(_data);
		}

		[[nodiscard]] auto IsUnset() const -> bool
		{
			return std::holds_alternative<StyleKeyword>(_data) &&
				   std::get<StyleKeyword>(_data) == StyleKeyword::Unset;
		}

		[[nodiscard]] auto IsInherited() const -> bool
		{
			return std::holds_alternative<StyleKeyword>(_data) &&
				   std::get<StyleKeyword>(_data) == StyleKeyword::Inherit;
		}

		[[nodiscard]] auto IsInitial() const -> bool
		{
			return std::holds_alternative<StyleKeyword>(_data) &&
				   std::get<StyleKeyword>(_data) == StyleKeyword::Initial;
		}

		template <typename Prop>
		auto Resolve(const T& parentValue, const Prop& prop) const -> T
		{
			if (IsValue())
			{
				return GetValue();
			}

			if (IsInherited())
			{
				return parentValue;
			}

			if (IsInitial())
			{
				return prop.DefaultValue;
			}

			return prop.Inherits ? parentValue : prop.DefaultValue;
		}

		template <typename Prop>
		auto Resolve(
			const T& parentValue,
			const T& defaultValue,
			const Prop& prop
		) const -> T
		{
			if (IsValue())
			{
				return GetValue();
			}

			if (IsInherited())
			{
				return parentValue;
			}

			if (IsInitial())
			{
				return defaultValue;
			}

			return prop.Inherits ? parentValue : defaultValue;
		}

	private:
		std::variant<T, StyleKeyword> _data;
	};

	enum class LengthUnit : char
	{
		Pixels,
		Percent
	};

	struct Length
	{
	public:
		LengthUnit Unit{};
		float Value{0};

		Length() = default;
		Length(float pixels) : Value(pixels) {};
		Length(LengthUnit unit, float value) : Unit(unit), Value(value) {};
	};

	template <typename T> struct Edges
	{
		T Left;
		T Top;
		T Right;
		T Bottom;
	};

	template <typename T> struct Axes
	{
		T Horizontal;
		T Vertical;
	};

	template <typename T> struct DeclaredEdges
	{
		StyleValue<T> Left;
		StyleValue<T> Top;
		StyleValue<T> Right;
		StyleValue<T> Bottom;

		template <typename Prop>
		auto Resolve(const Edges<T>& parentValue, const Prop& prop) const
			-> Edges<T>
		{
			Edges<T> edges;

			edges.Left =
				Left.Resolve(parentValue.Left, prop.DefaultValue.Left, prop);
			edges.Top =
				Top.Resolve(parentValue.Top, prop.DefaultValue.Top, prop);
			edges.Right =
				Right.Resolve(parentValue.Right, prop.DefaultValue.Right, prop);
			edges.Bottom = Bottom.Resolve(
				parentValue.Bottom,
				prop.DefaultValue.Bottom,
				prop
			);

			return edges;
		}
	};

	template <typename T> struct DeclaredAxes
	{
		StyleValue<T> Horizontal;
		StyleValue<T> Vertical;

		template <typename Prop>
		auto Resolve(const Axes<T>& parentValue, const Prop& prop) const
			-> Axes<T>
		{
			Axes<T> axes;

			axes.Horizontal = Horizontal.Resolve(
				parentValue.Horizontal,
				prop.DefaultValue.Horizontal,
				prop
			);
			axes.Vertical = Vertical.Resolve(
				parentValue.Vertical,
				prop.DefaultValue.Vertical,
				prop
			);

			return axes;
		}
	};

	namespace styles
	{
		namespace detail
		{
			struct AutoTag
			{
			};
			struct MaxContentTag
			{
			};
			struct FitContentTag
			{
			};
			struct StretchTag
			{
			};
		}

		inline constexpr detail::AutoTag Auto{};
		inline constexpr detail::MaxContentTag MaxContent{};
		inline constexpr detail::FitContentTag FitContent{};
		inline constexpr detail::StretchTag Stretch{};
	}

	template <typename T>
	using AutoValue = std::variant<T, styles::detail::AutoTag>;

	using SizingValue = std::variant<
		Length,
		styles::detail::AutoTag,
		styles::detail::MaxContentTag,
		styles::detail::FitContentTag,
		styles::detail::StretchTag>;

	using SizingValueNoAuto = std::variant<
		Length,
		styles::detail::MaxContentTag,
		styles::detail::FitContentTag,
		styles::detail::StretchTag>;

	struct DeclaredStyle
	{
	public:
#undef mc_styleProperty
#define mc_styleProperty(name, type, inherits, initial, declType, ...)         \
	declType name;
		mc_layoutProperties mc_renderProperties
#undef mc_styleProperty
	};

	struct ComputedStyle
	{
	public:
#define mc_styleProperty(name, type, ...) type name;
		mc_layoutProperties mc_renderProperties
#undef mc_styleProperty

			static auto
			Cascade(const DeclaredStyle& declared, const ComputedStyle& parent)
				-> ComputedStyle;
	};

	namespace styles
	{
		inline static ComputedStyle DefaultStyle = {
#define mc_styleProperty(name, type, inherits, initial, ...) .name = (initial),
			mc_layoutProperties mc_renderProperties
#undef mc_styleProperty
		};

		namespace detail::properties
		{
			template <typename T> struct StyleProperty
			{
			public:
				std::string_view Name;
				std::string_view Type;
				bool Inherits;
				T DefaultValue;
			};

#define mc_styleProperty(name, type, inherits, initial, ...)                   \
	static const StyleProperty<type> name##Property{                           \
		.Name = #name,                                                         \
		.Type = #type,                                                         \
		.Inherits = (inherits),                                                \
		.DefaultValue = (initial)                                              \
	};
			mc_layoutProperties mc_renderProperties
#undef mc_styleProperty
		}

		namespace detail::applying::layout
		{
#define mc_styleProperty(name, type, inherits, initial, ...)                   \
	void apply##name(YGNodeRef ref, type value);
			mc_layoutProperties
#undef mc_styleProperty
		}
	}
}
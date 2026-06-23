#pragma once
#include "Logger.h"
#include "styles.def"
#include "yoga/YGConfig.h"
#include <variant>

namespace mocca
{
	enum class StyleKeyword : char
	{
		Unset,
		Inherit,
		Initial
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

		Length(float pixels = 0) : Value(pixels) {};
		Length(LengthUnit unit, float value) : Unit(unit), Value(value) {};
	};

	namespace styles
	{
		inline auto px(float value) -> Length
		{
			return {value};
		}

		inline auto percent(float value) -> Length
		{
			return {LengthUnit::Percent, value};
		}

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

	using SizingValue = std::variant<
		Length,
		styles::detail::AutoTag,
		styles::detail::MaxContentTag,
		styles::detail::FitContentTag,
		styles::detail::StretchTag>;

	struct DeclaredStyle
	{
	public:
#undef mc_styleProperty
#define mc_styleProperty(name, type, ...) StyleValue<type> name;
		mc_layoutProperties
#undef mc_styleProperty
	};

	struct ComputedStyle
	{
	public:
#define mc_styleProperty(name, type, ...) type name;
		mc_layoutProperties
#undef mc_styleProperty

			static auto
			Cascade(const DeclaredStyle& declared, const ComputedStyle& parent)
				-> ComputedStyle;
	};

	namespace styles
	{
		inline static ComputedStyle DefaultStyle = {
#define mc_styleProperty(name, type, inherits, initial) .name = (initial),
			mc_layoutProperties
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

#define mc_styleProperty(name, type, inherits, initial)                        \
	static const StyleProperty<type> name##Property{                           \
		.Name = #name,                                                         \
		.Type = #type,                                                         \
		.Inherits = (inherits),                                                \
		.DefaultValue = (initial)                                              \
	};
			mc_layoutProperties
#undef mc_styleProperty
		}

		namespace detail::applying
		{
#define mc_styleProperty(name, type, inherits, initial) void apply##name(YGNodeRef ref, type value);
			mc_layoutProperties
#undef mc_styleProperty
		}
	}
}
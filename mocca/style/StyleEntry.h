#pragma once
#include "StyleProperty.h"
#include "StyleValue.h"
#include "Length.h"
#include "style/StyleEnums.h"
#include "utils/Color.h"
#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <variant>

namespace mocca::styles
{
	enum class StyleValueType : char
	{
		Keyword,
		Value,
		AuxKeyword,
		Enum,
		Length,
		Color,
		Handle,
		FontStyle,
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
			uint8_t Enum;
			struct Length Length;
			struct Color Color;
			uint32_t Handle = 0;
			FontStyleType FontStyle;
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

	template <> struct StyleEntryReader<uint32_t>
	{
		static auto Read(const StyleEntry& s) -> std::uint32_t
		{
			return s.Payload.Handle;
		}
	};

	template <> struct StyleEntryReader<FontStyleType>
	{
		static auto Read(const StyleEntry& s) -> FontStyleType
		{
			return s.Payload.FontStyle;
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
			if (s.Type == StyleValueType::AuxKeyword)
			{
				return s.Aux;
			}
			return StyleEntryReader<T>::Read(s);
		}
	};

	template <> struct StyleEntryReader<LineHeightType>
	{
		static auto Read(const StyleEntry& s) -> LineHeightType
		{
			if (s.Type == StyleValueType::AuxKeyword)
			{
				return s.Aux;
			}
			
			if (s.Type == StyleValueType::Value)
			{
				return s.Payload.Float;
			}

			return s.Payload.Length;
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

	inline auto entryEqual(const StyleEntry& a, const StyleEntry& b) -> bool
	{
		if (a.Prop != b.Prop || a.Type != b.Type)
		{
			return false;
		}

		const auto& x = a.Payload;
		const auto& y = b.Payload;
		switch (a.Type)
		{
		case StyleValueType::Keyword:
			return x.Keyword == y.Keyword;
		case StyleValueType::Value:
			return x.Float == y.Float;
		case StyleValueType::Enum:
			return x.Enum == y.Enum;
		case StyleValueType::Length:
			return x.Length.Value == y.Length.Value &&
				   x.Length.Unit == y.Length.Unit;
		case StyleValueType::Color:
			return std::memcmp(&x.Color, &y.Color, sizeof(Color)) == 0;
		case StyleValueType::Handle:
			return x.Handle == y.Handle;
		case StyleValueType::FontStyle:
			return x.FontStyle == y.FontStyle;
		case StyleValueType::AuxKeyword:
			return a.Aux == b.Aux;
		}
		return false;
	}
}

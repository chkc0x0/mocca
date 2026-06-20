#pragma once
#include "Logger.h"
#include <variant>

namespace mocca
{
	enum class StyleKeyword : char
	{
		Unset,
		Inherit
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
			return _data.index() == 0;
		}

		[[nodiscard]] auto IsUnset() const -> bool
		{
			return _data.index() == 1 &&
				   std::get<StyleKeyword>(_data) == StyleKeyword::Unset;
		}

		[[nodiscard]] auto IsInherited() const -> bool
		{
			return _data.index() == 1 &&
				   std::get<StyleKeyword>(_data) == StyleKeyword::Inherit;
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
	}

	struct DeclaredStyle
	{
	public:
		StyleValue<Length> Width;
		StyleValue<Length> Height;
	};
}
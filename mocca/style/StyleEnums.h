#pragma once

namespace mocca::styles
{
	enum class StyleKeyword : char
	{
		Unset,
		Inherit,
		Initial,
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
		End,
	};

	enum class DisplayType : char
	{
		Flex,
		None,
		Contents,
		Grid,
	};

	enum class FlexDirections : char
	{
		Column,
		ColumnReverse,
		Row,
		RowReverse,
	};

	enum class WrappingType : char
	{
		None,
		Wrap,
		WrapReverse,
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
		End,
	};

	enum class LayoutDirections : char
	{
		LTR,
		RTL,
	};

	enum class OverflowType : char
	{
		Visible,
		Hidden,
		Scroll,
	};

	enum class TextAlignment : char
	{
		Start,
		End,
		Left,
		Right,
		Center,
		Justify,
        MatchParent,
	};

	enum class FontStyleEnum : char
	{
		Normal,
		Italic,
		Oblique,
	};

	struct FontStyleType
	{
		FontStyleEnum Style;
		float Angle = 0;

		FontStyleType(FontStyleEnum style) : Style(style) {};
		FontStyleType(FontStyleEnum style, float angle) : Style(style), Angle(angle) {};
	};
}
#pragma once
#include "styles.def"
#include <array>
#include <cstddef>
#include <cstdint>

namespace mocca::styles
{
	enum class StyleProperty : char
	{
		None,
#undef mc_styleProperty
#define mc_styleProperty(name, ...) name,
		mc_textProperties mc_layoutProperties mc_renderProperties
			mc_miscProperties
#undef mc_styleProperty
				StylePropertyCount,
		All = StylePropertyCount,
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

	constexpr auto GroupBit(StylePropertyGroup g) -> std::uint8_t
	{
		return static_cast<std::uint8_t>(1U << static_cast<unsigned>(g));
	}

	inline constexpr std::size_t StylePropertyCount =
		static_cast<std::size_t>(StyleProperty::StylePropertyCount);

	inline constexpr std::array<std::uint8_t, StylePropertyCount> GroupBitsOf
		= {
			  0U,
#undef mc_styleProperty
#define mc_styleProperty(name, ...) GroupBit(StylePropertyGroup::mc_styleGroup),
#define mc_styleGroup Inherited
			  mc_textPropertiesInh
#undef mc_styleGroup
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
#define mc_styleGroup Inherited
			  mc_layoutPropertiesInh
#undef mc_styleGroup
#define mc_styleGroup Render
			  mc_renderPropertiesReset
#undef mc_styleGroup
#define mc_styleGroup Inherited
			  mc_miscPropertiesInh
#undef mc_styleGroup
#undef mc_styleProperty
		  };

	inline constexpr std::uint8_t AllGroups = (1U << 6U) - 1U;
	inline constexpr std::uint8_t InheritedGroupBit =
		GroupBit(StylePropertyGroup::Inherited);
	inline constexpr std::uint8_t RenderGroupBit =
		GroupBit(StylePropertyGroup::Render);
	inline constexpr std::uint8_t LayoutGroups =
		static_cast<std::uint8_t>(AllGroups & ~RenderGroupBit);
	inline constexpr std::uint8_t PaintGroups = RenderGroupBit | InheritedGroupBit;
	inline constexpr std::uint8_t DecorationGroups =
		static_cast<std::uint8_t>(
			PaintGroups | GroupBit(StylePropertyGroup::LayoutSpacing)
		);
}

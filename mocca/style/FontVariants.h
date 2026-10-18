#pragma once
#include "style/FontStyleTypes.h"
#include <concepts>
#include <cstdint>
#include <vector>

namespace mocca::styles
{
	enum class FontVariantLigature : char
	{
		Normal,
		None,
		Common,
		NoCommon,
		Discretionary,
		NoDiscretionary,
		Historical,
		NoHistorical,
		Contextual,
		NoContextual,
	};

	enum class FontVariantNumeric : char
	{
		Normal,
		Ordinal,
		SlashedZero,
		Lining,
		Oldstyle,
		Proportional,
		Tabular,
		DiagonalFractions,
		StackedFractions,
	};

	namespace detail
	{
		inline void addFontFeature(
			std::vector<FontFeaturesType::Value>& out,
			const char (&tag)[5],
			uint32_t value
		)
		{
			out.push_back({.Tag=FontTag(tag), .Value=value});
		}

		inline void expandFontVariant(
			std::vector<FontFeaturesType::Value>& out,
			FontVariantLigature kw
		)
		{
			switch (kw)
			{
			case FontVariantLigature::Normal:
				break;
			case FontVariantLigature::None:
				addFontFeature(out, "liga", 0U);
				addFontFeature(out, "clig", 0U);
				addFontFeature(out, "calt", 0U);
				addFontFeature(out, "dlig", 0U);
				addFontFeature(out, "hlig", 0U);
				break;
			case FontVariantLigature::Common:
				addFontFeature(out, "liga", 1U);
				addFontFeature(out, "clig", 1U);
				break;
			case FontVariantLigature::NoCommon:
				addFontFeature(out, "liga", 0U);
				addFontFeature(out, "clig", 0U);
				break;
			case FontVariantLigature::Discretionary:
				addFontFeature(out, "dlig", 1U);
				break;
			case FontVariantLigature::NoDiscretionary:
				addFontFeature(out, "dlig", 0U);
				break;
			case FontVariantLigature::Historical:
				addFontFeature(out, "hlig", 1U);
				break;
			case FontVariantLigature::NoHistorical:
				addFontFeature(out, "hlig", 0U);
				break;
			case FontVariantLigature::Contextual:
				addFontFeature(out, "calt", 1U);
				break;
			case FontVariantLigature::NoContextual:
				addFontFeature(out, "calt", 0U);
				break;
			}
		}

		inline void expandFontVariant(
			std::vector<FontFeaturesType::Value>& out,
			FontVariantNumeric kw
		)
		{
			switch (kw)
			{
			case FontVariantNumeric::Normal:
				break;
			case FontVariantNumeric::Ordinal:
				addFontFeature(out, "ordn", 1U);
				break;
			case FontVariantNumeric::SlashedZero:
				addFontFeature(out, "zero", 1U);
				break;
			case FontVariantNumeric::Lining:
				addFontFeature(out, "lnum", 1U);
				break;
			case FontVariantNumeric::Oldstyle:
				addFontFeature(out, "onum", 1U);
				break;
			case FontVariantNumeric::Proportional:
				addFontFeature(out, "pnum", 1U);
				break;
			case FontVariantNumeric::Tabular:
				addFontFeature(out, "tnum", 1U);
				break;
			case FontVariantNumeric::DiagonalFractions:
				addFontFeature(out, "frac", 1U);
				break;
			case FontVariantNumeric::StackedFractions:
				addFontFeature(out, "afrc", 1U);
				break;
			}
		}
	}

	template <class... Keywords>
		requires(std::same_as<Keywords, FontVariantLigature> && ...)
	inline auto FontVariantLigatures(Keywords... kws) -> FontFeaturesType
	{
		std::vector<FontFeaturesType::Value> out;
		(detail::expandFontVariant(out, kws), ...);
		return FontFeaturesType{out};
	}

	template <class... Keywords>
		requires(std::same_as<Keywords, FontVariantNumeric> && ...)
	inline auto FontVariantNumeric(Keywords... kws) -> FontFeaturesType
	{
		std::vector<FontFeaturesType::Value> out;
		(detail::expandFontVariant(out, kws), ...);
		return FontFeaturesType{out};
	}
}

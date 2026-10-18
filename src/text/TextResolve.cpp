#include "TextResolve.h"
#include "style/StyleValue.h"
#include <variant>

namespace mocca::mtext::detail
{
	const std::vector<std::string> DefaultFamilies{"sans-serif"};

	auto ResolveFamilies(const styles::ComputedStyle& style)
		-> std::vector<std::string>
	{
		uint32_t handle = style.Get<styles::StyleProperty::FontFamily>();
		if (handle == 0)
		{
			return DefaultFamilies;
		}

		return styles::detail::resolveFontFamily(handle).Families;
	}

	auto ResolveFontRequest(const styles::ComputedStyle& style) -> FontRequest
	{
		FontRequest request;
		request.Weight = style.Get<styles::StyleProperty::FontWeight>();
		request.Stretch = style.Get<styles::StyleProperty::FontStretch>();

		const FontStyleType& fontStyle =
			style.Get<styles::StyleProperty::FontStyle>();
		request.Style = fontStyle.Style;
		request.Slant = fontStyle.Angle;

		return request;
	}

	auto ResolveTextLayoutStyle(const styles::ComputedStyle& style)
		-> TextLayoutStyle
	{
		using P = styles::StyleProperty;

		TextLayoutStyle out;
		out.Families = ResolveFamilies(style);
		out.Request = ResolveFontRequest(style);
		out.SizePx = style.Get<P::FontSize>().Value;
		out.Variations = style.Get<P::FontVariationSettings>();
		out.Features = style.Get<P::FontFeatureSettings>();
		out.Align = style.Get<P::TextAlign>();

		if (const auto* spacing =
				std::get_if<Length>(&style.Get<P::LetterSpacing>()))
		{
			out.LetterSpacingPx = spacing->Value;
		}

		if (const auto* spacing =
				std::get_if<Length>(&style.Get<P::WordSpacing>()))
		{
			out.WordSpacingPx = spacing->Value;
		}

		const styles::LineHeightType& lineHeight = style.Get<P::LineHeight>();
		if (std::holds_alternative<float>(lineHeight))
		{
			out.LineHeight = LineHeightKind::Number;
			out.LineHeightValue = std::get<float>(lineHeight);
		}
		else if (std::holds_alternative<Length>(lineHeight))
		{
			out.LineHeight = LineHeightKind::Px;
			out.LineHeightValue = std::get<Length>(lineHeight).Value;
		}

		return out;
	}
}

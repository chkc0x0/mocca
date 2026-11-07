#include "style/StyleApply.h"
#include "gfx/Decoration.h"
#include "style/StyleValue.h"
#include <algorithm>

namespace mocca::styles::detail
{
	auto resolveLengthPx(const Length& value, float basis) -> float
	{
		return value.Unit == LengthUnit::Percent ? value.Value * basis / 100.0F
												 : value.Value;
	}
}

namespace mocca::styles::detail::applying::paint
{
	auto anyBorder(const EdgeSizes& border) -> bool
	{
		return border.Top > 0 || border.Right > 0 || border.Bottom > 0 ||
			   border.Left > 0;
	}

	auto clampRadius(float radius, float width, float height) -> float
	{
		float limit = std::min(width, height) * 0.5F;
		return std::max(0.0F, std::min(radius, limit));
	}

	auto resolveRadius(
		const Length& value,
		const PaintContext& ctx,
		bool& sizeDependent
	) -> float
	{
		if (value.Unit == LengthUnit::Percent)
		{
			sizeDependent = true;
		}

		return resolveLengthPx(value, std::min(ctx.Width, ctx.Height));
	}

	auto
	resolveBorderWidth(const Length& value, float basis, bool& sizeDependent)
		-> float
	{
		if (value.Unit == LengthUnit::Percent)
		{
			sizeDependent = true;
		}

		return std::max(0.0F, resolveLengthPx(value, basis));
	}

	void applyBackgroundColor(
		gfx::BoxDecoration& d,
		const Color& value,
		const PaintContext&
	)
	{
		d.HasFill = value.A > 0;
		d.Fill = gfx::Paint::Fill(value);
	}

	void applyBorderColor(
		gfx::BoxDecoration& d,
		const Color& value,
		const PaintContext&
	)
	{
		d.BorderColor = value;
	}

	void applyBorderRadiusTopLeft(
		gfx::BoxDecoration& d,
		const Length& value,
		const PaintContext& ctx
	)
	{
		d.Radius.TopLeft = resolveRadius(value, ctx, d.SizeDependent);
	}

	void applyBorderRadiusTopRight(
		gfx::BoxDecoration& d,
		const Length& value,
		const PaintContext& ctx
	)
	{
		d.Radius.TopRight = resolveRadius(value, ctx, d.SizeDependent);
	}

	void applyBorderRadiusBottomLeft(
		gfx::BoxDecoration& d,
		const Length& value,
		const PaintContext& ctx
	)
	{
		d.Radius.BottomLeft = resolveRadius(value, ctx, d.SizeDependent);
	}

	void applyBorderRadiusBottomRight(
		gfx::BoxDecoration& d,
		const Length& value,
		const PaintContext& ctx
	)
	{
		d.Radius.BottomRight = resolveRadius(value, ctx, d.SizeDependent);
	}

	void applyBackgroundImage(
		gfx::BoxDecoration& d,
		const uint32_t& value,
		const PaintContext& ctx
	)
	{
		if (value == 0)
		{
			return;
		}

		const auto& image = detail::resolveBackgroundImage(value);
		d.Images.clear();
		d.Images.reserve(image.Layers.size());
		for (const auto& layer : image.Layers)
		{
			d.Images.push_back(layer.Texture);
		}
		d.HasImage = !d.Images.empty();
	}

	void
	applyTransition(gfx::BoxDecoration&, const uint32_t&, const PaintContext&)
	{
	}

	void applyBorderTop(
		gfx::BoxDecoration& d,
		const Length& value,
		const PaintContext& ctx
	)
	{
		d.Border.Top = resolveBorderWidth(value, ctx.Height, d.SizeDependent);
		d.HasBorder = d.BorderColor.A > 0 && anyBorder(d.Border);
	}

	void applyBorderRight(
		gfx::BoxDecoration& d,
		const Length& value,
		const PaintContext& ctx
	)
	{
		d.Border.Right = resolveBorderWidth(value, ctx.Width, d.SizeDependent);
		d.HasBorder = d.BorderColor.A > 0 && anyBorder(d.Border);
	}

	void applyBorderBottom(
		gfx::BoxDecoration& d,
		const Length& value,
		const PaintContext& ctx
	)
	{
		d.Border.Bottom =
			resolveBorderWidth(value, ctx.Height, d.SizeDependent);
		d.HasBorder = d.BorderColor.A > 0 && anyBorder(d.Border);
	}

	void applyBorderLeft(
		gfx::BoxDecoration& d,
		const Length& value,
		const PaintContext& ctx
	)
	{
		d.Border.Left = resolveBorderWidth(value, ctx.Width, d.SizeDependent);
		d.HasBorder = d.BorderColor.A > 0 && anyBorder(d.Border);
	}

	void normalizeRadius(gfx::BoxDecoration& d, float width, float height)
	{
		CornerRadii& radius = d.Radius;

		radius.TopLeft = clampRadius(radius.TopLeft, width, height);
		radius.TopRight = clampRadius(radius.TopRight, width, height);
		radius.BottomRight = clampRadius(radius.BottomRight, width, height);
		radius.BottomLeft = clampRadius(radius.BottomLeft, width, height);

		float scale = 1.0F;
		if (radius.TopLeft + radius.TopRight > 0)
		{
			scale = std::min(scale, width / (radius.TopLeft + radius.TopRight));
		}
		if (radius.BottomLeft + radius.BottomRight > 0)
		{
			scale = std::min(
				scale,
				width / (radius.BottomLeft + radius.BottomRight)
			);
		}
		if (radius.TopLeft + radius.BottomLeft > 0)
		{
			scale =
				std::min(scale, height / (radius.TopLeft + radius.BottomLeft));
		}
		if (radius.TopRight + radius.BottomRight > 0)
		{
			scale = std::min(
				scale,
				height / (radius.TopRight + radius.BottomRight)
			);
		}

		if (scale < 1.0F)
		{
			radius.TopLeft *= scale;
			radius.TopRight *= scale;
			radius.BottomRight *= scale;
			radius.BottomLeft *= scale;
		}
	}
}

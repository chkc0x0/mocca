#pragma once
#include "utils/Color.h"
#include <cstdint>

namespace mocca::gfx
{
	enum class PaintStyle : uint8_t
	{
		Fill,
		Stroke,
		FillAndStroke,
	};

	enum class StrokeCap : uint8_t
	{
		Butt,
		Round,
		Square,
	};

	enum class StrokeJoin : uint8_t
	{
		Miter,
		Round,
		Bevel,
	};

	struct Paint
	{
	public:
		mocca::Color Color = colors::Transparent;
		PaintStyle Style = PaintStyle::Fill;
		float StrokeWidth = 1.0F;
		float MiterLimit = 4.0F;
		StrokeCap Cap = StrokeCap::Butt;
		StrokeJoin Join = StrokeJoin::Miter;
		bool AntiAlias = true;
		float Opacity = 1.0F;
		
		static auto Fill(const mocca::Color& color) -> Paint;
		static auto Stroke(const mocca::Color& color, float width) -> Paint;

		[[nodiscard]] auto HasStroke() const -> bool
		{
			return Style != PaintStyle::Fill;
		}
	};

	inline auto Paint::Fill(const mocca::Color& color) -> Paint
	{
		Paint paint;
		paint.Color = color;
		paint.Style = PaintStyle::Fill;
		return paint;
	}

	inline auto Paint::Stroke(const mocca::Color& color, float width) -> Paint
	{
		Paint paint;
		paint.Color = color;
		paint.Style = PaintStyle::Stroke;
		paint.StrokeWidth = width;
		return paint;
	}
}

#pragma once
#include "text/FontTypes.h"
#include "style/StyleEnums.h"
#include "utils/Math.h"
#include <cstdint>
#include <string>
#include <vector>

namespace mocca::mtext
{
	enum class LineHeightKind : uint8_t
	{
		Normal,
		Number,
		Px,
	};

	struct ShapedGlyph
	{
		uint32_t GlyphId = 0;
		float X = 0;
		float XAdvance = 0;
		float XOffset = 0;
		float YOffset = 0;
		uint32_t Cluster = 0;
	};

	struct ShapedRun
	{
		FontFaceId Face = SIZE_MAX;
		float Size = 0;
		std::vector<ShapedGlyph> Glyphs;
		float Width = 0;
	};

	struct TextLine
	{
		uint32_t Start = 0;
		uint32_t End = 0;
		float Width = 0;
		float Baseline = 0;
		uint32_t RunBegin = 0;
		uint32_t RunEnd = 0;

		[[nodiscard]] auto XOffset(float boxWidth, TextAlignment align) const
			-> float;
	};

	struct TextLayout
	{
		FontFaceId Face = SIZE_MAX;
		float FontSize = 0;
		float Ascent = 0;
		float Descent = 0;
		float LineHeight = 0;
		std::vector<ShapedRun> Runs;
		std::vector<TextLine> Lines;
		Vector2 Size{.X = 0, .Y = 0};
	};

	struct TextLayoutStyle
	{
		std::vector<std::string> Families;
		FontRequest Request;
		float SizePx = 0;
		float LetterSpacingPx = 0;
		float WordSpacingPx = 0;

		LineHeightKind LineHeight = LineHeightKind::Normal;
		float LineHeightValue = 0;

		uint32_t Variations = 0;
		uint32_t Features = 0;
		TextAlignment Align = TextAlignment::Start;
	};

	namespace detail
	{
		struct LineEmitter
		{
			TextLayout& Layout;
			const ShapedText& Shaped;
			const TextLayoutStyle& Style;
			const std::vector<Cluster>& Clusters;
			float Scale = 0;
			float LineHeight = 0;
			float HalfLeading = 0;
			float Ascent = 0;

			[[nodiscard]] auto Emit(size_t begin, size_t end) -> float;
		};
	}
}

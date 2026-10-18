#pragma once
#include "text/FontTypes.h"
#include "style/StyleEnums.h"
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
}

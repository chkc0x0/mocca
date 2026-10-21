#pragma once
#include "text/FontTypes.h"
#include "text/TextLayout.h"

namespace mocca::mtext::detail
{
	[[nodiscard]] auto buildClusters(
		const ShapedText& shaped,
		const TextLayoutStyle& style
	) -> std::vector<Cluster>;

	[[nodiscard]] inline auto isSpaceByte(char c) -> bool
	{
		return c == ' ';
	}

	[[nodiscard]] inline auto isHangingByte(char c) -> bool
	{
		return c == ' ' || c == '\t' || c == '\r' || c == '\n';
	}

	[[nodiscard]] auto breakOpportunities(std::string_view utf8)
		-> std::vector<BreakFlag>;

	[[nodiscard]] auto resolveBidi(std::string_view utf8, BaseDirection base)
		-> BidiResult;
}
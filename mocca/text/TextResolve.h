#pragma once
#include "style/ComputedStyle.h"
#include "text/TextLayout.h"
#include <string>
#include <vector>

namespace mocca::mtext::detail
{
	auto resolveFamilies(const styles::ComputedStyle& style)
		-> std::vector<std::string>;

	auto resolveFontRequest(const styles::ComputedStyle& style) -> FontRequest;

	auto resolveTextLayoutStyle(const styles::ComputedStyle& style)
		-> TextLayoutStyle;
}

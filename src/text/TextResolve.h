#pragma once
#include "style/ComputedStyle.h"
#include "text/TextLayout.h"
#include <string>
#include <vector>

namespace mocca::mtext::detail
{
	auto ResolveFamilies(const styles::ComputedStyle& style)
		-> std::vector<std::string>;

	auto ResolveFontRequest(const styles::ComputedStyle& style) -> FontRequest;

	auto ResolveTextLayoutStyle(const styles::ComputedStyle& style)
		-> TextLayoutStyle;
}

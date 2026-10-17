#pragma once
#include "text/FontTypes.h"
#include <cstddef>
#include <span>

namespace mocca::mtext
{
	struct FontMatchResult
	{
		size_t Index = SIZE_MAX;
		float Weight = 400;
		float Stretch = 100;
		float Slant = 0;

		explicit operator bool() const
		{
			return Index != SIZE_MAX;
		}
	};

	class FontMatcher
	{
	public:
		static auto Best(
			std::span<const FontDescriptor*> candidates,
			const FontRequest& q
		) -> FontMatchResult;
	};
}

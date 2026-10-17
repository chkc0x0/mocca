#pragma once
#include "core/Identifiers.h"
#include <functional>
#include <string>
#include <vector>

namespace mocca::styles
{
	struct FontFamilyType
	{
	public:
		std::vector<std::string> Families;

		constexpr auto operator==(const FontFamilyType&) const
			-> bool = default;
	};
}

namespace std
{
	template <> struct hash<mocca::styles::FontFamilyType>
	{
		auto operator()(const mocca::styles::FontFamilyType& f) const noexcept
			-> size_t
		{
			size_t h = std::hash<size_t>{}(f.Families.size());
			for (const std::string& name : f.Families)
			{
				mocca::hashCombine(h, std::hash<std::string>{}(name));
			}
			return h;
		}
	};
}

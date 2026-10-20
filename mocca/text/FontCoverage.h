#pragma once
#include "hb.h"
#include "text/FontTypes.h"
#include <cstdint>
#include <memory>
#include <unordered_map>

namespace mocca::mtext::detail
{
	using SetPtr = std::unique_ptr<hb_set_t, decltype(&hb_set_destroy)>;

	class FontCoverage
	{
	public:
		[[nodiscard]] static auto Covers(FontFaceId face, uint32_t codepoint)
			-> bool;

		static void Clear();

	private:
		static auto SetFor(FontFaceId face) -> const hb_set_t*;
		inline static std::unordered_map<FontFaceId, SetPtr> sets;
	};
}

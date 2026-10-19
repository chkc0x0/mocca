#pragma once
#include "FontTypes.h"
#include "ShapeCache.h"
#include "TextLayout.h"
#include "core/Identifiers.h"
#include "hb.h"
#include <cstdint>
#include <memory>
#include <string_view>
#include <unordered_map>

namespace mocca::mtext::detail
{
	using FontPtr = std::unique_ptr<hb_font_t, decltype(&hb_font_destroy)>;

	struct FontKey
	{
		FontFaceId Face = SIZE_MAX;
		size_t Variations = 0;

		auto operator==(const FontKey&) const -> bool = default;
	};

	struct FontKeyHash
	{
		auto operator()(const FontKey& key) const -> size_t
		{
			size_t hash = std::hash<size_t>{}(key.Face);
			mocca::hashCombine(hash, key.Variations);
			return hash;
		}
	};

	struct TextShaperStats
	{
		uint64_t ShapeCalls = 0;
		uint64_t ShapeHits = 0;
	};

	class TextShaper
	{
	public:
		static auto Shape(
			std::string_view content,
			const TextLayoutStyle& style
		) -> std::shared_ptr<const ShapedText>;

		static void PurgeCache();
		static auto GetStats() -> TextShaperStats;

	private:
		static auto ReportNoFace() -> std::shared_ptr<const ShapedText>;
		static auto FontFor(
			hb_face_t* face,
			FontFaceId id,
			const std::vector<hb_variation_t>& variations
		) -> hb_font_t*;

		inline static TextShaperStats stats;
		inline static ShapeCache cache{64};
		inline static std::unordered_map<FontKey, FontPtr, FontKeyHash> fonts;
		inline static uint64_t nextShapedId = 1;
		inline static bool warnedNoFace = false;
	};
}
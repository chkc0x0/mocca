#pragma once
#include "text/FontTypes.h"
#include "text/LayoutCache.h"
#include "text/TextLayout.h"
#include <cstdint>
#include <memory>

namespace mocca::styles
{
	class ComputedStyle;
}

namespace mocca::mtext::detail
{
	struct LayoutStats
	{
		uint64_t LayoutBuilds = 0;
		uint64_t LayoutHits = 0;
	};

	class TextLayoutBuilder
	{
	public:
		static auto BuildLayout(
			const ShapedText& shaped,
			const TextLayoutStyle& style,
			float maxWidth,
			bool wrap
		) -> std::shared_ptr<const TextLayout>;

		static auto LayoutFor(
			const std::shared_ptr<const ShapedText>& shaped,
			const TextLayoutStyle& style,
			float maxWidth,
			bool wrap
		) -> std::shared_ptr<const TextLayout>;

		auto ResolveTextLayout(
			const styles::ComputedStyle& style,
			std::string_view content,
			float maxWidth,
			bool wrap
		) -> std::shared_ptr<const TextLayout>;

		static void PurgeCache();
		static auto GetStats() -> LayoutStats;

	private:
		inline static LayoutCache cache{64};
		inline static LayoutStats stats;
	};
}

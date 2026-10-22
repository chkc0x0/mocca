#pragma once
#include "utils/Range.h"
#include "style/StyleEnums.h"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <variant>
#include <vector>

namespace mocca::mtext
{
	// x macros!
#define mc_fontProperty(name, type)
#define mc_fontProperties                                                      \
	mc_fontProperty(Weight, Range<float>)                                      \
		mc_fontProperty(Stretch, Range<float>)                                 \
			mc_fontProperty(Style, styles::FontStyleEnum)                      \
				mc_fontProperty(Slant, Range<float>)

	struct FaceProps
	{
#undef mc_fontProperty
#define mc_fontProperty(name, type) type name;
		mc_fontProperties
#undef mc_fontProperty
	};

	struct FontDescriptor : FaceProps
	{
		std::string Family;
		uint32_t FaceIndex = 0;

		auto operator==(const FontDescriptor& b) const -> bool
		{
#undef mc_fontProperty
#define mc_fontProperty(name, type) name == b.name&&
			return mc_fontProperties Family == b.Family &&
				   FaceIndex == b.FaceIndex;
#undef mc_fontProperty
		}
	};

	struct FontDescriptorOverrides
	{
		std::optional<std::string> Family;
		std::optional<uint32_t> FaceSelector;
#undef mc_fontProperty
#define mc_fontProperty(name, type) std::optional<type> name;
		mc_fontProperties
#undef mc_fontProperty
	};

	struct FontMemorySource
	{
		std::span<const std::byte> Data;
		bool Copy = false;
	};

	using FontSource = std::variant<std::string, FontMemorySource>;
	using FontFaceId = size_t;

	struct FontRequest
	{
		float Weight = 400.0F;
		float Stretch = 100.0F;
		styles::FontStyleEnum Style = styles::FontStyleEnum::Normal;
		float Slant = 14.0F;
	};

	struct FontMatch
	{
		FontFaceId Id = SIZE_MAX;
		float Weight = 400;
		float Stretch = 100;
		float Slant = 0;

		explicit operator bool() const
		{
			return Id != SIZE_MAX;
		}
	};

	struct FontStackEntry
	{
		std::string Requested;
		std::string Resolved;
		bool ViaAlias = false;
		FontMatch Match;
	};

	struct FontStack
	{
		std::vector<FontStackEntry> Entries;
		FontRequest Request;

		[[nodiscard]] auto Faces() const
		{
			return Entries | std::ranges::views::filter(
								 [](auto& e) -> auto { return bool(e.Match); }
							 );
		}
	};

	namespace detail
	{
		struct ShapeGlyph
		{
			uint32_t GlyphId = 0;
			int32_t XAdvance = 0;
			int32_t YAdvance = 0;
			int32_t XOffset = 0;
			int32_t YOffset = 0;
			uint32_t Cluster = 0;
		};

		struct ShapedItem
		{
			FontFaceId Face = SIZE_MAX;
			uint32_t Upem = 1000;
			uint32_t Start = 0;
			uint32_t End = 0;
			uint8_t Level = 0;
			int32_t Ascender = 0;
			int32_t Descender = 0;
			int32_t LineGap = 0;
			std::vector<ShapeGlyph> Glyphs;
		};

		struct ShapedParagraph
		{
			uint32_t Start = 0;
			uint32_t End = 0;
			uint8_t BaseLevel = 0;
		};

		struct ShapedText
		{
			uint64_t Id = 0;
			uint32_t Variations = 0;
			uint32_t Features = 0;
			std::string Content;

			FontFaceId StrutFace = SIZE_MAX;
			uint32_t StrutUpem = 1000;
			int32_t StrutAscender = 0;
			int32_t StrutDescender = 0;
			int32_t StrutLineGap = 0;

			std::vector<ShapedItem> Items;
			std::vector<ShapedParagraph> Paragraphs;
		};

		struct TextItem
		{
			uint32_t Start = 0;
			uint32_t End = 0;
			uint32_t Script = 0;
			uint8_t Level = 0;
			FontFaceId Face = SIZE_MAX;

			static auto Itemize(
				std::string_view content,
				std::span<const FontFaceId> stack,
				std::span<const uint8_t> levels
			) -> std::vector<TextItem>;
		};

		struct Grapheme
		{
			uint32_t Start = 0;
			uint32_t End = 0;
			uint32_t Script = 0;
			bool Sticky = false;
			std::vector<uint32_t> Codepoints;
		};

		struct BidiResult
		{
			std::vector<uint8_t> Levels;
			std::vector<ShapedParagraph> Paragraphs;
			bool UsedEngine = false;
		};
	}

	struct Cluster
	{
		uint32_t Start = 0;
		uint32_t End = 0;
		uint32_t Item = 0;
		uint32_t GlyphBegin = 0;
		uint32_t GlyphEnd = 0;
		float Advance = 0;
		bool IsSpace = false;
		bool Hanging = false;
	};

	enum class BreakFlag : uint8_t
	{
		No = 0,
		Allow = 1,
		Must = 2,
	};
}

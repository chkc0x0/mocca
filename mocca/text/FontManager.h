#pragma once
#include "utils/Range.h"
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace mocca::mtext
{
	enum class FontStyle : uint8_t
	{
		Normal,
		Italic,
		Oblique,
	};

	// x macros!
#define mc_fontProperty(name, type)
#define mc_fontProperties                                                      \
	mc_fontProperty(Family, std::string) mc_fontProperty(Weight, Range<float>) \
		mc_fontProperty(Stretch, Range<float>)                                 \
			mc_fontProperty(Style, FontStyle)                                  \
				mc_fontProperty(Slant, Range<float>)                           \
					mc_fontProperty(FaceIndex, uint32_t)

	struct FontDescriptor
	{
#undef mc_fontProperty
#define mc_fontProperty(name, type) type name;
		mc_fontProperties
#undef mc_fontProperty

			auto operator==(const FontDescriptor& b) const -> bool
		{
#undef mc_fontProperty
#define mc_fontProperty(name, type) name == b.name&&
			return mc_fontProperties FaceIndex == b.FaceIndex;
#undef mc_fontProperty
		}
	};

	struct FontDescriptorOverrides
	{
#undef mc_fontProperty
#define mc_fontProperty(name, type) std::optional<type> name;
		mc_fontProperties
#undef mc_fontProperty
	};

	using FontSource = std::variant<std::string, std::span<const std::byte>>;

	class FontManager
	{
	public:
		using FontFaceId = size_t;

		struct FontEntry
		{
			FontFaceId Id;
			FontDescriptor Desc;
			FontSource Source;
		};

		void RegisterFile(
			const std::string& path,
			FontDescriptorOverrides overrides = {}
		);

	private:
		static auto LoadFontFaces(
			const FontSource& source,
			int32_t requestedIndex,
			std::string_view fallbackFamily
		) -> std::vector<FontDescriptor>;
		std::vector<FontEntry> _entries;
		std::unordered_map<std::string, std::vector<FontFaceId>> _families;
	};
}
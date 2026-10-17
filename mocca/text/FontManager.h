#pragma once
#include "utils/Range.h"
#include <cstdint>
#include <optional>
#include <ranges>
#include <span>
#include "style/StyleEnums.h"
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace mocca::mtext
{
	// x macros!
#define mc_fontProperty(name, type)
#define mc_fontProperties                                                      \
	mc_fontProperty(Weight, Range<float>)                                      \
		mc_fontProperty(Stretch, Range<float>)                                 \
			mc_fontProperty(Style, FontStyleEnum)                              \
				mc_fontProperty(Slant, Range<float>)                           \
					mc_fontProperty(FaceIndex, uint32_t)

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

		auto operator==(const FontDescriptor& b) const -> bool
		{
#undef mc_fontProperty
#define mc_fontProperty(name, type) name == b.name&&
			return mc_fontProperties Family == b.Family;
#undef mc_fontProperty
		}
	};

	struct FontDescriptorOverrides
	{
		std::optional<std::string> Family;
#undef mc_fontProperty
#define mc_fontProperty(name, type) std::optional<type> name;
		mc_fontProperties
#undef mc_fontProperty
	};

	using FontSource = std::variant<std::string, std::span<const std::byte>>;
	using FontFaceId = size_t;

	struct FontRequest
	{
		float Weight = 400.0F;
		float Stretch = 100.0F;
		FontStyleEnum Style = FontStyleEnum::Normal;
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

	class FontManager
	{
	public:
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

		auto MatchIn(
			const std::vector<FontFaceId>& ids,
			const FontRequest& q
		) const -> FontMatch;

		auto MatchStack(
			const std::vector<std::string>& families,
			const FontRequest& q
		) const -> FontStack;

		auto Match(std::string_view family, const FontRequest& q) const
			-> FontMatch;

		void
		SetAlias(const std::string_view& alias, const std::string_view& family);
		void Inspect(const FontStack& s) const;

	private:
		static auto LoadFontFaces(
			const FontSource& source,
			int32_t requestedIndex,
			std::string_view fallbackFamily
		) -> std::vector<FontDescriptor>;

		std::vector<FontEntry> _entries;
		std::unordered_map<std::string, std::vector<FontFaceId>> _families;
		std::unordered_map<std::string, std::string> _aliases;
	};
}
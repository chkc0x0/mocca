#include "text/FontManager.h"
#include "core/Logger.h"
#include <algorithm>
#include <filesystem>
#include <utility>
#include <unistd.h>
#include "HarfbuzzUtils.h"

namespace mocca::mtext
{
	static auto NormalizeFamily(std::string_view s) -> std::string
	{
		std::string out(s);
		std::transform(
			out.begin(),
			out.end(),
			out.begin(),
			[](unsigned char c) -> char { return char(std::tolower(c)); }
		);
		return out;
	}

	void FontManager::RegisterFile(
		const std::string& path,
		FontDescriptorOverrides overrides
	)
	{
		if (access(path.c_str(), F_OK | R_OK) != 0)
		{
			mc_error(
				ErrorCode::InvalidArgument,
				"font file {} either does not exist or cant be read from",
				path
			);
			return;
		}

		FontSource source = path;
		auto faces = LoadFontFaces(
			source,
			static_cast<int>(overrides.FaceIndex.value_or(-1)),
			std::filesystem::path(path).stem().string()
		);

		if (faces.empty())
		{
			mc_error(
				ErrorCode::InvalidArgument,
				"font file {} contains no readable faces",
				path
			);
			return;
		}

		for (FontDescriptor& descr : faces)
		{
			auto sniffedIndex = descr.FaceIndex;

#undef mc_fontProperty
#define mc_fontProperty(name, type)                                            \
	if (overrides.name)                                                        \
	{                                                                          \
		descr.name = overrides.name.value();                                   \
	}
			mc_fontProperties
#undef mc_fontProperty

				if (overrides.Family)
			{
				descr.Family = overrides.Family.value();
			}

			descr.FaceIndex = sniffedIndex;

			auto& family = _families[NormalizeFamily(descr.Family)];

			bool duplicate = std::any_of(
				family.begin(),
				family.end(),
				[&](auto id) -> auto { return _entries[id].Desc == descr; }
			);
			if (duplicate)
			{
				mc_error(
					ErrorCode::InvalidArgument,
					"font face {} (index {}) from {} is already registered",
					descr.Family,
					descr.FaceIndex,
					path
				);
				continue;
			}

			FontEntry entry;
			entry.Id = _entries.size();
			entry.Desc = std::move(descr);
			entry.Source = path;

			family.push_back(entry.Id);
			_entries.push_back(std::move(entry));

			mc_debug(
				"added entry ({}) to font family {} from file",
				_entries.back().Id,
				_entries.back().Desc.Family
			);
		}
	}

	auto FontManager::LoadFontFaces(
		const FontSource& source,
		int32_t requestedIndex,
		std::string_view fallbackFamily
	) -> std::vector<FontDescriptor>
	{
		detail::BlobPtr blob = detail::makeBlob(source);
		if (hb_blob_get_length(blob.get()) == 0)
		{
			return {};
		}

		unsigned count = hb_face_count(blob.get());
		if (count == 0 || std::cmp_greater_equal(requestedIndex, count))
		{
			return {};
		}

		std::vector<FontDescriptor> out;
		if (requestedIndex >= 0)
		{
			out.push_back(
				detail::sniffFace(blob.get(), requestedIndex, fallbackFamily)
			);
		}
		else
		{
			for (unsigned i = 0; i < count; i++)
			{
				out.push_back(detail::sniffFace(blob.get(), i, fallbackFamily));
			}
		}
		return out;
		return out;
	}

	namespace detail
	{
		struct Rank
		{
			uint8_t Tier;
			float Dist;
			auto operator<=>(const Rank&) const = default;
		};

		constexpr Rank WorstRank{
			.Tier = 255,
			.Dist = std::numeric_limits<float>::infinity(),
		};

		auto nearest(const Range<float>& r, float v) -> float
		{
			return std::clamp(v, r.Lo, r.Hi);
		}

		auto stretchRank(const Range<float>& r, float d) -> Rank
		{
			if (r.Contains(d))
			{
				return {.Tier = 0, .Dist = 0};
			}
			float v = nearest(r, d);
			bool narrower = v < d;
			bool narrowerFirst = d <= 100.0F;
			return {
				.Tier = static_cast<uint8_t>(narrower == narrowerFirst ? 1 : 2),
				.Dist = std::abs(v - d),
			};
		}

		auto weightRank(const Range<float>& r, float d) -> Rank
		{
			if (r.Contains(d))
			{
				return {.Tier = 0, .Dist = 0};
			}
			float v = nearest(r, d);
			float dist = std::abs(v - d);

			if (d >= 400.0F && d <= 500.0F)
			{
				if (v > d && v <= 500.0F)
				{
					return {.Tier = 1, .Dist = dist};
				}
				if (v < d)
				{
					return {.Tier = 2, .Dist = dist};
				}
				return {.Tier = 3, .Dist = dist};
			}
			if (d < 400.0F)
			{
				return {
					.Tier = static_cast<uint8_t>(v < d ? 1 : 2),
					.Dist = dist,
				};
			}
			return {
				.Tier = static_cast<uint8_t>(v > d ? 1 : 2),
				.Dist = dist,
			};
		}

		auto styleRank(const FaceProps& p, const FontRequest& q) -> Rank
		{
			static constexpr uint8_t Order[3][3] = {
				{0, 2, 1}, // normal
				{2, 0, 1}, // italic
				{2, 1, 0}, // oblique
			};

			uint8_t tier = Order[int(q.Style)][int(p.Style)];
			float dist = (p.Style == FontStyleEnum::Oblique)
							 ? std::abs(nearest(p.Slant, q.Slant) - q.Slant)
							 : 0.0F;
			return {.Tier = tier, .Dist = dist};
		}
	}

	auto FontManager::MatchIn(
		const std::vector<FontFaceId>& ids,
		const FontRequest& q
	) const -> FontMatch
	{
		auto stretchOf = [&](FontFaceId id) -> detail::Rank
		{ return detail::stretchRank(_entries[id].Desc.Stretch, q.Stretch); };
		auto styleOf = [&](FontFaceId id) -> detail::Rank
		{ return detail::styleRank(_entries[id].Desc, q); };
		auto weightOf = [&](FontFaceId id) -> detail::Rank
		{ return detail::weightRank(_entries[id].Desc.Weight, q.Weight); };

		detail::Rank s = detail::WorstRank;
		detail::Rank y = detail::WorstRank;
		detail::Rank w = detail::WorstRank;

		for (const auto& id : ids)
		{
			s = std::min(s, stretchOf(id));
		}
		for (const auto& id : ids)
		{
			if (stretchOf(id) == s)
			{
				y = std::min(y, styleOf(id));
			}
		}
		for (const auto& id : ids)
		{
			if (stretchOf(id) == s && styleOf(id) == y)
			{
				w = std::min(w, weightOf(id));
			}
		}

		for (const auto& id : ids)
		{
			if (stretchOf(id) != s || styleOf(id) != y || weightOf(id) != w)
			{
				continue;
			}

			const auto& d = _entries[id].Desc;
			FontMatch m;
			m.Id = id;
			m.Weight = detail::nearest(d.Weight, q.Weight);
			m.Stretch = detail::nearest(d.Stretch, q.Stretch);
			m.Slant = (d.Style == FontStyleEnum::Oblique &&
					   q.Style == FontStyleEnum::Oblique)
						  ? detail::nearest(d.Slant, q.Slant)
						  : 0.0F;
			return m;
		}
		return {};
	}

	auto FontManager::MatchStack(
		const std::vector<std::string>& families,
		const FontRequest& q
	) const -> FontStack
	{
		FontStack out;
		out.Request = q;
		out.Entries.reserve(families.size());

		for (const auto& name : families)
		{
			FontStackEntry e;
			e.Requested = name;
			e.Resolved = NormalizeFamily(name);

			if (auto a = _aliases.find(e.Resolved); a != _aliases.end())
			{
				e.Resolved = a->second;
				e.ViaAlias = true;
			}

			if (auto it = _families.find(e.Resolved); it != _families.end())
			{
				e.Match = MatchIn(it->second, q);
			}

			out.Entries.push_back(std::move(e));
		}
		return out;
	}

	auto FontManager::Match(std::string_view family, const FontRequest& q) const
		-> FontMatch
	{
		std::string one(family);
		auto stack = MatchStack({one}, q);
		return stack.Entries[0].Match;
	}

	void FontManager::SetAlias(
		const std::string_view& alias,
		const std::string_view& family
	)
	{
		_aliases[NormalizeFamily(alias)] = NormalizeFamily(family);
	}

	void FontManager::Inspect(const FontStack& s) const
	{
		for (const auto& e : s.Entries)
		{
			if (!e.Match)
			{
				mc_debug("  {:<14} -> not registered", e.Requested);
				continue;
			}
			const auto& d = _entries[e.Match.Id].Desc;
			mc_debug(
				"  {:<14} -> #{} {} w{:g} s{:g} {}{}",
				e.Requested,
				e.Match.Id,
				d.Family,
				e.Match.Weight,
				e.Match.Stretch,
				d.Style == FontStyleEnum::Normal   ? "normal"
				: d.Style == FontStyleEnum::Italic ? "italic"
												   : "oblique",
				e.ViaAlias ? "  (alias)" : ""
			);
		}
	}
}
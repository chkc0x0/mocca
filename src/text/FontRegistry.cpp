#include "text/FontRegistry.h"
#include "text/FontMatcher.h"
#include "core/Logger.h"
#include "HarfbuzzUtils.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <utility>
#include <unistd.h>

namespace mocca::mtext
{
	auto FontRegistry::Normalize(std::string_view s) -> std::string
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

	void FontRegistry::RegisterFile(
		const std::string& path,
		const FontDescriptorOverrides& overrides
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

		_registerSource(
			FontSource{path},
			std::filesystem::path(path).stem().string(),
			path,
			overrides
		);
	}

	void FontRegistry::RegisterMemory(
		std::span<const std::byte> data,
		const FontDescriptorOverrides& overrides
	)
	{
		if (data.empty())
		{
			mc_error(ErrorCode::InvalidArgument, "font memory buffer is empty");
			return;
		}

		_registerSource(
			FontSource{data},
			overrides.Family.value_or("<memory>"),
			"<memory>",
			overrides
		);
	}

	void FontRegistry::_registerSource(
		const FontSource& source,
		std::string_view fallbackFamily,
		std::string_view label,
		const FontDescriptorOverrides& overrides
	)
	{
		auto faces = detail::loadFaces(
			source,
			static_cast<int32_t>(overrides.FaceSelector.value_or(-1)),
			fallbackFamily
		);

		if (faces.empty())
		{
			mc_error(
				ErrorCode::InvalidArgument,
				"font {} contains no readable faces",
				label
			);
			return;
		}

		for (FontDescriptor& descr : faces)
		{
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

			auto& family = _families[Normalize(descr.Family)];

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
					label
				);
				continue;
			}

			FontEntry entry;
			entry.Id = _entries.size();
			entry.Desc = std::move(descr);
			entry.Source = source;

			family.push_back(entry.Id);
			_entries.push_back(std::move(entry));

			mc_debug(
				"added entry ({}) to font family {} from file",
				_entries.back().Id,
				_entries.back().Desc.Family
			);
		}
	}

	auto FontRegistry::MatchIn(
		std::span<const FontFaceId> ids,
		const FontRequest& q
	) const -> FontMatch
	{
		std::vector<const FontDescriptor*> candidates;
		candidates.reserve(ids.size());
		for (auto id : ids)
		{
			candidates.push_back(&_entries[id].Desc);
		}

		auto best = FontMatcher::Best(candidates, q);
		if (!best)
		{
			return {};
		}

		FontMatch m;
		m.Id = ids[best.Index];
		m.Weight = best.Weight;
		m.Stretch = best.Stretch;
		m.Slant = best.Slant;
		return m;
	}

	auto FontRegistry::MatchStack(
		std::span<const std::string> families,
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
			e.Resolved = Normalize(name);

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

	auto FontRegistry::Match(
		std::string_view family,
		const FontRequest& q
	) const -> FontMatch
	{
		std::string one(family);
		auto stack = MatchStack(std::span<const std::string>(&one, 1), q);
		return stack.Entries[0].Match;
	}

	void FontRegistry::SetAlias(std::string_view alias, std::string_view family)
	{
		_aliases[Normalize(alias)] = Normalize(family);
	}

	void FontRegistry::DumpStack(const FontStack& s) const
	{
#ifdef DEBUG
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
#else
		mc_error(ErrorCode::InvalidState, "DumpStack is debug only");
#endif
	}
}

#pragma once
#include "text/FontTypes.h"
#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace mocca::mtext
{
	class FontRegistry
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
			const FontDescriptorOverrides& overrides = {}
		);

		void RegisterMemory(
			std::span<const std::byte> data,
			const FontDescriptorOverrides& overrides = {}
		);

		void SetAlias(std::string_view alias, std::string_view family);

		auto MatchIn(
			std::span<const FontFaceId> ids,
			const FontRequest& q
		) const -> FontMatch;

		auto MatchStack(
			std::span<const std::string> families,
			const FontRequest& q
		) const -> FontStack;

		auto Match(std::string_view family, const FontRequest& q) const
			-> FontMatch;

		void DumpStack(const FontStack& s) const;

		[[nodiscard]] auto Entry(FontFaceId id) const -> const FontEntry&
		{
			return _entries[id];
		}

	private:
		static auto Normalize(std::string_view s) -> std::string;

		void _registerSource(
			const FontSource& source,
			std::string_view fallbackFamily,
			std::string_view label,
			const FontDescriptorOverrides& overrides
		);

		std::vector<FontEntry> _entries;
		std::unordered_map<std::string, std::vector<FontFaceId>> _families;
		std::unordered_map<std::string, std::string> _aliases;
	};
}

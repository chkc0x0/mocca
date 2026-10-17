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
			[](unsigned char c) { return char(std::tolower(c)); }
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
			overrides.FaceIndex.value_or(-1),
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
}
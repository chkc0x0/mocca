#include "HarfbuzzUtils.h"
#include "utils/Overloaded.h"
#include <algorithm>
#include <string_view>
#include <utility>
#include <vector>

namespace mocca::mtext::detail
{
	auto makeBlob(const FontSource& source) -> BlobPtr
	{
		return std::visit(
			overloaded{
				[](const std::string& path) -> BlobPtr
				{
					return {
						hb_blob_create_from_file(path.c_str()),
						hb_blob_destroy
					};
				},
				[](const FontMemorySource& src) -> BlobPtr
				{
					return {
						hb_blob_create(
							reinterpret_cast<const char*>(src.Data.data()),
							static_cast<unsigned>(src.Data.size()),
							src.Copy ? HB_MEMORY_MODE_DUPLICATE
									 : HB_MEMORY_MODE_READONLY,
							nullptr,
							nullptr
						),
						hb_blob_destroy
					};
				},
			},
			source
		);
	}

	auto getName(hb_face_t* face, hb_ot_name_id_t id) -> std::string
	{
		unsigned len = hb_ot_name_get_utf8(
			face,
			id,
			HB_LANGUAGE_INVALID,
			nullptr,
			nullptr
		);
		if (len == 0)
		{
			return {};
		}
		std::string s(len, '\0');
		unsigned size = len + 1;
		hb_ot_name_get_utf8(face, id, HB_LANGUAGE_INVALID, &size, s.data());
		s.resize(size);
		return s;
	}

	auto
	sniffFace(hb_blob_t* blob, uint32_t index, std::string_view fallbackFamily)
		-> LoadedFace
	{
		LoadedFace out;
		out.Face.reset(hb_face_create(blob, index));

		hb_face_t* face = out.Face.get();
		FontDescriptor& d = out.Desc;
		d.FaceIndex = index;

		d.Family = getName(face, HB_OT_NAME_ID_TYPOGRAPHIC_FAMILY);
		if (d.Family.empty())
		{
			d.Family = getName(face, HB_OT_NAME_ID_FONT_FAMILY);
		}
		if (d.Family.empty())
		{
			d.Family = std::string(fallbackFamily);
		}

		Table os2(face, HB_TAG('O', 'S', '/', '2'));
		bool italic = false;
		bool oblique = false;
		if (os2.Present())
		{
			uint16_t w = os2.U16(4).value_or(400);
			d.Weight = Range<float>(
				w == 0 ? 400.0F : std::clamp<float>(w, 1, 1000)
			);
			d.Stretch = Range<float>(
				widthClassToPercent(os2.U16(6).value_or(5))
			);

			uint16_t version = os2.U16(0).value_or(0);
			uint16_t sel = os2.U16(62).value_or(0);
			italic = ((sel & (1 << 0)) != 0);
			oblique = version >= 4 && ((sel & (1 << 9)) != 0);
		}
		else
		{
			Table head(face, HB_TAG('h', 'e', 'a', 'd'));
			italic = ((head.U16(44).value_or(0) & 2) != 0);
		}

		d.Style = oblique  ? styles::FontStyleEnum::Oblique
				  : italic ? styles::FontStyleEnum::Italic
						   : styles::FontStyleEnum::Normal;

		// css obliques are positive so we negate
		if (d.Style == styles::FontStyleEnum::Oblique)
		{
			Table post(face, HB_TAG('p', 'o', 's', 't'));
			float angle = -static_cast<float>(post.I32(4).value_or(0)) /
						  65536.0F;
			d.Slant = Range<float>(angle != 0.0F ? angle : 14.0F);
		}

		if (hb_ot_var_has_data(face) != 0)
		{
			hb_ot_var_axis_info_t axes[16];
			unsigned count = 16;
			hb_ot_var_get_axis_infos(face, 0, &count, axes);
			for (unsigned i = 0; i < count; i++)
			{
				const auto& a = axes[i];
				switch (a.tag)
				{
				case HB_OT_TAG_VAR_AXIS_WEIGHT:
					d.Weight = {a.min_value, a.max_value};
					break;
				case HB_OT_TAG_VAR_AXIS_WIDTH:
					d.Stretch = {a.min_value, a.max_value};
					break;
				case HB_OT_TAG_VAR_AXIS_SLANT:
					d.Slant = {-a.max_value, -a.min_value};
					break;
				default:
					break;
				}
			}
		}
		return out;
	}

	auto widthClassToPercent(uint16_t c) -> float
	{
		static constexpr float T[] =
			{50, 62.5F, 75, 87.5F, 100, 112.5F, 125, 150, 200};
		return (c >= 1 && c <= 9) ? T[c - 1] : 100.0F;
	}

	auto loadFaces(
		const FontSource& source,
		int32_t requestedIndex,
		std::string_view fallbackFamily
	) -> std::vector<LoadedFace>
	{
		BlobPtr blob = makeBlob(source);
		if (hb_blob_get_length(blob.get()) == 0)
		{
			return {};
		}

		unsigned count = hb_face_count(blob.get());
		if (count == 0 || std::cmp_greater_equal(requestedIndex, count))
		{
			return {};
		}

		std::vector<LoadedFace> out;
		if (requestedIndex >= 0)
		{
			out.push_back(
				sniffFace(blob.get(), requestedIndex, fallbackFamily)
			);
		}
		else
		{
			for (unsigned i = 0; i < count; i++)
			{
				out.push_back(sniffFace(blob.get(), i, fallbackFamily));
			}
		}
		return out;
	}
}
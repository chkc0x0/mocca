#pragma once
#include "text/FontTypes.h"
#include <hb.h>
#include <hb-ot.h>
#include <memory>
#include <string_view>
#include <vector>

namespace mocca::mtext::detail
{
	using BlobPtr = std::unique_ptr<hb_blob_t, decltype(&hb_blob_destroy)>;
	using FacePtr = std::unique_ptr<hb_face_t, decltype(&hb_face_destroy)>;

	auto makeBlob(const FontSource& source) -> BlobPtr;

	class Table
	{
	public:
		Table(hb_face_t* face, hb_tag_t tag)
			: _blob(hb_face_reference_table(face, tag), hb_blob_destroy)
		{
			_data = reinterpret_cast<const uint8_t*>(
				hb_blob_get_data(_blob.get(), &_size)
			);
		}

		[[nodiscard]] auto Present() const -> bool
		{
			return _size != 0;
		}

		[[nodiscard]] auto U16(size_t off) const -> std::optional<uint16_t>
		{
			if (off + 2 > _size)
			{
				return std::nullopt;
			}
			return static_cast<uint16_t>(_data[off] << 8 | _data[off + 1]);
		}

		[[nodiscard]] auto I32(size_t off) const -> std::optional<int32_t>
		{
			if (off + 4 > _size)
			{
				return std::nullopt;
			}
			return static_cast<int32_t>(
				static_cast<uint32_t>(_data[off]) << 24 |
				static_cast<uint32_t>(_data[off + 1]) << 16 |
				static_cast<uint32_t>(_data[off + 2]) << 8 |
				static_cast<uint32_t>(_data[off + 3])
			);
		}

	private:
		BlobPtr _blob;
		const uint8_t* _data = nullptr;
		unsigned _size = 0;
	};

	auto getName(hb_face_t* face, hb_ot_name_id_t id) -> std::string;

	auto widthClassToPercent(uint16_t c) -> float;

	auto
	sniffFace(hb_blob_t* blob, uint32_t index, std::string_view fallbackFamily)
		-> FontDescriptor;

	auto loadFaces(
		const FontSource& source,
		int32_t requestedIndex,
		std::string_view fallbackFamily
	) -> std::vector<FontDescriptor>;
}
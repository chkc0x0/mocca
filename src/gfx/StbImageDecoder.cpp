#include "stb_image.h"

#include "gfx/StbImageDecoder.h"

namespace mocca::gfx
{
	auto StbImageDecoder::Name() const -> std::string_view
	{
		return "stb";
	}

	auto StbImageDecoder::Probe(std::span<const std::byte> data, ImageInfo& out)
		const -> bool
	{
		int width = 0;
		int height = 0;
		int channels = 0;

		const auto* bytes = reinterpret_cast<const stbi_uc*>(data.data());
		if (stbi_info_from_memory(
				bytes,
				static_cast<int>(data.size()),
				&width,
				&height,
				&channels
			) == 0)
		{
			return false;
		}

		if (width <= 0 || height <= 0)
		{
			return false;
		}

		out.Width = static_cast<uint32_t>(width);
		out.Height = static_cast<uint32_t>(height);
		return true;
	}

	auto StbImageDecoder::Decode(std::span<const std::byte> data, DecodedImage& out)
		const -> bool
	{
		int width = 0;
		int height = 0;
		int channels = 0;

		const auto* bytes = reinterpret_cast<const stbi_uc*>(data.data());
		stbi_uc* pixels = stbi_load_from_memory(
			bytes,
			static_cast<int>(data.size()),
			&width,
			&height,
			&channels,
			4
		);

		if (pixels == nullptr)
		{
			return false;
		}

		auto count = static_cast<std::size_t>(width) *
						   static_cast<std::size_t>(height) * 4;

		out.Width = static_cast<uint32_t>(width);
		out.Height = static_cast<uint32_t>(height);
		out.Format = TextureFormat::RGBA8;
		const auto* begin = reinterpret_cast<const std::byte*>(pixels);
		out.Pixels.assign(begin, begin + count);

		stbi_image_free(pixels);
		return true;
	}
}

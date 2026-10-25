#include "gfx/ImageDecoderRegistry.h"
#include <fstream>

namespace mocca::gfx
{
	void ImageDecoderRegistry::Register(std::unique_ptr<ImageDecoder> decoder)
	{
		decoders.insert(decoders.begin(), std::move(decoder));
	}

	auto ImageDecoderRegistry::Probe(
		std::span<const std::byte> data,
		ImageInfo& out
	) -> bool
	{
		for (const auto& decoder : decoders)
		{
			if (decoder->Probe(data, out))
			{
				return true;
			}
		}

		return false;
	}

	auto ImageDecoderRegistry::Decode(
		std::span<const std::byte> data,
		DecodedImage& out
	) -> bool
	{
		ImageInfo info;
		for (const auto& decoder : decoders)
		{
			if (decoder->Probe(data, info))
			{
				return decoder->Decode(data, out);
			}
		}

		return false;
	}

	auto ImageDecoderRegistry::DecodeFile(
		const std::string& path,
		DecodedImage& out
	) -> bool
	{
		std::ifstream file(path, std::ios::binary | std::ios::ate);
		if (!file)
		{
			return false;
		}

		auto size = file.tellg();
		if (size <= 0)
		{
			return false;
		}

		std::vector<std::byte> data(static_cast<std::size_t>(size));
		file.seekg(0);
		file.read(reinterpret_cast<char*>(data.data()), size);
		if (!file)
		{
			return false;
		}

		return Decode(data, out);
	}
}

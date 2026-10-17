#pragma once
#include "utils/Range.h"
#include <cstdint>
#include <string>

namespace mocca::mtext
{
	enum class FontStyle : uint8_t
	{
		Normal,
		Italic,
		Oblique,
	};
	
	struct FontDescriptor
	{
		std::string Family = "<unknown>";
		Range<float> Weight = 400;
		Range<float> Stretch = 100;
		FontStyle Style = FontStyle::Normal;
		Range<float> Slant = 0;
		uint32_t FaceIndex = 0;
	};

	class FontManager
	{
	public:
		void RegisterFile(const std::string_view& path, const FontDescriptor& desc = {});
	};
}
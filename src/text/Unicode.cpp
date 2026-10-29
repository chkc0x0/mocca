#include "text/Unicode.h"
#include "wordbreak.h"
#include "utf8proc.h"
#include <algorithm>
#include <cstddef>
#include <simdutf.h>
#include <vector>

namespace mocca::mtext
{
	struct Decoded
	{
		uint32_t Codepoint = 0xFFFD;
		uint32_t Length = 1;
	};

	auto decodeAt(std::string_view text, size_t offset) -> Decoded
	{
		utf8proc_int32_t cp = 0xFFFD;
		utf8proc_ssize_t length = utf8proc_iterate(
			reinterpret_cast<const utf8proc_uint8_t*>(text.data() + offset),
			static_cast<utf8proc_ssize_t>(text.size() - offset),
			&cp
		);
		if (length <= 0)
		{
			return Decoded{.Codepoint = 0xFFFD, .Length = 1};
		}

		return Decoded{
			.Codepoint = static_cast<uint32_t>(cp),
			.Length = static_cast<uint32_t>(length),
		};
	}

	constexpr uint32_t ZeroWidthJoiner = 0x200D;
	constexpr uint32_t Vs16 = 0xFE0F;
	constexpr uint32_t SkinToneFirst = 0x1F3FB;
	constexpr uint32_t SkinToneLast = 0x1F3FF;
	constexpr uint32_t RegionalFirst = 0x1F1E6;
	constexpr uint32_t RegionalLast = 0x1F1FF;

	auto isRegionalIndicator(uint32_t cp) -> bool
	{
		return cp >= RegionalFirst && cp <= RegionalLast;
	}

	auto keepsClusterWhole(uint32_t cp) -> bool
	{
		return cp == ZeroWidthJoiner || cp == Vs16 ||
			   (cp >= SkinToneFirst && cp <= SkinToneLast);
	}

	auto Unicode::Encode(char32_t cp) -> std::string
	{
		std::string result;
		if (cp >= 0xD800 && cp <= 0xDFFF)
		{
			return "\uFFFD";
		}
		if (cp > 0x10FFFF)
		{
			return "\uFFFD";
		}

		if (cp < 0x80)
		{
			result.push_back(static_cast<char>(cp));
		}
		else if (cp < 0x800)
		{
			result.push_back(static_cast<char>(0xC0 | (cp >> 6)));
			result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
		}
		else if (cp < 0x10000)
		{
			result.push_back(static_cast<char>(0xE0 | (cp >> 12)));
			result.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
			result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
		}
		else
		{
			result.push_back(static_cast<char>(0xF0 | (cp >> 18)));
			result.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
			result.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
			result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
		}

		return result;
	}

	auto Unicode::NextGraphemeBoundary(std::string_view text, uint32_t byte)
		-> uint32_t
	{
		auto size = static_cast<uint32_t>(text.size());
		if (byte >= size)
		{
			return size;
		}

		uint32_t offset = byte;
		utf8proc_int32_t state = 0;
		utf8proc_int32_t previous = -1;

		while (offset < size)
		{
			Decoded decoded = decodeAt(text, offset);
			if (previous != -1 &&
				utf8proc_grapheme_break_stateful(
					previous,
					static_cast<utf8proc_int32_t>(decoded.Codepoint),
					&state
				))
			{
				return offset;
			}

			previous = static_cast<utf8proc_int32_t>(decoded.Codepoint);
			offset += decoded.Length;
		}

		return size;
	}

	auto Unicode::PrevGraphemeBoundary(std::string_view text, uint32_t byte)
		-> uint32_t
	{
		auto size = static_cast<uint32_t>(text.size());
		if (byte == 0)
		{
			return 0;
		}

		uint32_t target = std::min(byte, size);
		uint32_t cursor = 0;
		while (cursor < size)
		{
			uint32_t next = NextGraphemeBoundary(text, cursor);
			if (next >= target)
			{
				return cursor;
			}
			cursor = next;
		}

		return size;
	}

	auto Unicode::IsGraphemeBoundary(std::string_view text, uint32_t byte)
		-> bool
	{
		if (byte == 0 || byte == text.size())
		{
			return true;
		}
		if (byte > text.size())
		{
			return false;
		}

		return NextGraphemeBoundary(text, byte - 1) == byte;
	}

	auto Unicode::PrevBackspaceBoundary(std::string_view text, uint32_t byte)
		-> uint32_t
	{
		if (byte == 0)
		{
			return 0;
		}

		uint32_t start = PrevGraphemeBoundary(text, byte);
		uint32_t end = NextGraphemeBoundary(text, start);

		if (end - start == 2 && text[start] == '\r' && text[start + 1] == '\n')
		{
			return start;
		}

		bool whole = false;
		uint32_t count = 0;
		uint32_t regional = 0;
		uint32_t lastCodepoint = start;

		uint32_t cursor = start;
		while (cursor < end)
		{
			Decoded decoded = decodeAt(text, cursor);
			lastCodepoint = cursor;
			count++;
			whole = whole || keepsClusterWhole(decoded.Codepoint);
			if (isRegionalIndicator(decoded.Codepoint))
			{
				regional++;
			}
			cursor += decoded.Length;
		}

		if (whole || regional >= 2)
		{
			return start;
		}
		if (count > 1)
		{
			return lastCodepoint;
		}

		return start;
	}

	auto wordBreaks(std::string_view text) -> std::vector<char>
	{
		static const bool Initialized = [] -> bool
		{
			init_wordbreak();
			return true;
		}();
		(void)Initialized;

		std::vector<char> breaks(text.size(), WORDBREAK_NOBREAK);
		if (!text.empty())
		{
			set_wordbreaks_utf8(
				reinterpret_cast<const utf8_t*>(text.data()),
				text.size(),
				"en",
				breaks.data()
			);
		}
		return breaks;
	}

	auto Unicode::NextWordBoundary(std::string_view text, uint32_t byte)
		-> uint32_t
	{
		auto size = static_cast<uint32_t>(text.size());
		if (byte >= size)
		{
			return size;
		}

		std::vector<char> breaks = wordBreaks(text);
		for (size_t i = byte; i < size; i++)
		{
			if (breaks[i] == WORDBREAK_BREAK)
			{
				return static_cast<uint32_t>(i + 1);
			}
		}

		return size;
	}

	auto Unicode::PrevWordBoundary(std::string_view text, uint32_t byte)
		-> uint32_t
	{
		auto size = static_cast<uint32_t>(text.size());
		if (byte == 0)
		{
			return 0;
		}

		uint32_t target = std::min(byte, size);
		std::vector<char> breaks = wordBreaks(text);

		uint32_t result = 0;
		for (size_t i = 0; i + 1 < target; i++)
		{
			if (breaks[i] == WORDBREAK_BREAK)
			{
				result = static_cast<uint32_t>(i + 1);
			}
		}

		return result;
	}

	auto Unicode::CountGraphemes(std::string_view text) -> uint32_t
	{
		auto size = static_cast<uint32_t>(text.size());
		uint32_t count = 0;
		uint32_t cursor = 0;
		while (cursor < size)
		{
			cursor = NextGraphemeBoundary(text, cursor);
			count++;
		}
		return count;
	}

	auto Unicode::SanitizeUtf8(std::string_view text) -> std::string
	{
		if (text.empty())
		{
			return {};
		}

		if (simdutf::validate_utf8(text.data(), text.size()))
		{
			return std::string(text);
		}

		std::string out;
		out.reserve(text.size());

		size_t offset = 0;
		while (offset < text.size())
		{
			utf8proc_int32_t cp = 0xFFFD;
			utf8proc_ssize_t length = utf8proc_iterate(
				reinterpret_cast<const utf8proc_uint8_t*>(text.data() + offset),
				static_cast<utf8proc_ssize_t>(text.size() - offset),
				&cp
			);
			if (length <= 0)
			{
				out += "\uFFFD";
				offset += 1;
				continue;
			}

			out.append(text.data() + offset, static_cast<size_t>(length));
			offset += static_cast<size_t>(length);
		}

		return out;
	}
}

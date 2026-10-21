#include "text/TextUtils.h"
#include "SheenBidi/SheenBidi.h"
#include "utf8proc.h"
#include <span>

namespace mocca::mtext::detail
{
	auto hasRightToLeft(std::string_view utf8) -> bool
	{
		const auto* data = reinterpret_cast<const utf8proc_uint8_t*>(
			utf8.data()
		);
		size_t offset = 0;

		while (offset < utf8.size())
		{
			utf8proc_int32_t codepoint = 0;
			utf8proc_ssize_t length = utf8proc_iterate(
				data + offset,
				static_cast<utf8proc_ssize_t>(utf8.size() - offset),
				&codepoint
			);
			if (length <= 0)
			{
				offset++;
				continue;
			}
			offset += static_cast<size_t>(length);

			switch (utf8proc_get_property(codepoint)->bidi_class)
			{
			case UTF8PROC_BIDI_CLASS_R:
			case UTF8PROC_BIDI_CLASS_AL:
			case UTF8PROC_BIDI_CLASS_AN:
			case UTF8PROC_BIDI_CLASS_LRE:
			case UTF8PROC_BIDI_CLASS_LRO:
			case UTF8PROC_BIDI_CLASS_RLE:
			case UTF8PROC_BIDI_CLASS_RLO:
			case UTF8PROC_BIDI_CLASS_PDF:
			case UTF8PROC_BIDI_CLASS_LRI:
			case UTF8PROC_BIDI_CLASS_RLI:
			case UTF8PROC_BIDI_CLASS_FSI:
			case UTF8PROC_BIDI_CLASS_PDI:
				return true;
			default:
				break;
			}
		}

		return false;
	}

	auto splitParagraphs(
		std::string_view utf8,
		std::span<const uint8_t> baseForByte
	) -> std::vector<ShapedParagraph>
	{
		std::vector<ShapedParagraph> paragraphs;
		size_t start = 0;
		uint8_t previous = 0;

		for (size_t i = 0; i <= utf8.size(); i++)
		{
			if (i < utf8.size() && utf8[i] != '\n')
			{
				continue;
			}

			uint8_t base = (start < baseForByte.size()) ? baseForByte[start]
														: previous;
			paragraphs.push_back(
				ShapedParagraph{
					.Start = static_cast<uint32_t>(start),
					.End = static_cast<uint32_t>(i),
					.BaseLevel = base,
				}
			);
			previous = base;
			start = i + 1;
		}

		return paragraphs;
	}

	auto requestedLevel(BaseDirection base) -> SBLevel
	{
		switch (base)
		{
		case BaseDirection::LTR:
			return 0;
		case BaseDirection::RTL:
			return 1;
		case BaseDirection::Auto:
		default:
			return SBLevelDefaultLTR;
		}
	}

	auto resolveBidi(std::string_view utf8, BaseDirection base) -> BidiResult
	{
		BidiResult result;
		result.Levels.assign(utf8.size(), 0);

		if (utf8.empty() ||
			(base != BaseDirection::RTL && !hasRightToLeft(utf8)))
		{
			result.Paragraphs = splitParagraphs(utf8, result.Levels);
			return result;
		}

		SBCodepointSequence sequence{
			.stringEncoding = SBStringEncodingUTF8,
			.stringBuffer = utf8.data(),
			.stringLength = utf8.size(),
		};

		SBAlgorithmRef algorithm = SBAlgorithmCreate(&sequence);
		if (algorithm == nullptr)
		{
			result.Paragraphs = splitParagraphs(utf8, result.Levels);
			return result;
		}

		result.UsedEngine = true;
		std::vector<uint8_t> baseForByte(utf8.size(), 0);
		SBLevel requested = requestedLevel(base);

		SBUInteger offset = 0;
		while (offset < utf8.size())
		{
			SBUInteger length = 0;
			SBUInteger separator = 0;
			SBAlgorithmGetParagraphBoundary(
				algorithm,
				offset,
				utf8.size() - offset,
				&length,
				&separator
			);
			if (length == 0)
			{
				length = utf8.size() - offset;
			}

			SBParagraphRef paragraph = SBAlgorithmCreateParagraph(
				algorithm,
				offset,
				length,
				requested
			);
			if (paragraph != nullptr)
			{
				SBLevel baseLevel = SBParagraphGetBaseLevel(paragraph);
				const SBLevel* levels = SBParagraphGetLevelsPtr(paragraph);
				SBUInteger count = SBParagraphGetLength(paragraph);

				for (SBUInteger i = 0; i < count; i++)
				{
					result.Levels[offset + i] = levels[i];
					baseForByte[offset + i] = baseLevel;
				}

				SBParagraphRelease(paragraph);
			}

			offset += length;
		}

		SBAlgorithmRelease(algorithm);

		result.Paragraphs = splitParagraphs(utf8, baseForByte);
		return result;
	}
}

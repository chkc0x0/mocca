#include "text/FontTypes.h"
#include "core/Logger.h"
#include "hb.h"
#include "text/FontCoverage.h"
#include "utf8proc.h"
#include <algorithm>
#include <utility>

namespace mocca::mtext::detail
{
	auto isIgnorable(uint32_t cp) -> bool
	{
		if (cp < 0x20 || (cp >= 0x7F && cp <= 0x9F))
		{
			return true;
		}
		if (cp == 0x00AD || cp == 0x200B || cp == 0x200C || cp == 0x200D ||
			cp == 0x2060 || cp == 0x2028 || cp == 0x2029 || cp == 0xFEFF)
		{
			return true;
		}
		return (cp >= 0xFE00 && cp <= 0xFE0F) ||
			   (cp >= 0xE0100 && cp <= 0xE01EF);
	}

	auto isSeparator(const Grapheme& grapheme) -> bool
	{
		return std::ranges::find(grapheme.Codepoints, 0x0AU) !=
			   grapheme.Codepoints.end();
	}

	auto hasRequirements(const Grapheme& grapheme) -> bool
	{
		return std::ranges::any_of(
			grapheme.Codepoints,
			[](uint32_t cp) -> auto { return !isIgnorable(cp); }
		);
	}

	auto splitGraphemes(std::string_view content) -> std::vector<Grapheme>
	{
		hb_unicode_funcs_t* funcs = hb_unicode_funcs_get_default();
		const auto* data = reinterpret_cast<const utf8proc_uint8_t*>(
			content.data()
		);
		size_t size = content.size();
		size_t offset = 0;
		utf8proc_int32_t state = 0;
		utf8proc_int32_t previous = -1;

		std::vector<Grapheme> out;
		Grapheme current;

		while (offset < size)
		{
			utf8proc_int32_t cp = 0xFFFD;
			utf8proc_ssize_t length = utf8proc_iterate(
				data + offset,
				static_cast<utf8proc_ssize_t>(size - offset),
				&cp
			);
			if (length <= 0)
			{
				length = 1;
				cp = 0xFFFD;
			}

			if (previous != -1 &&
				utf8proc_grapheme_break_stateful(previous, cp, &state) &&
				!current.Codepoints.empty())
			{
				out.push_back(std::move(current));
				current = Grapheme{};
			}

			if (current.Codepoints.empty())
			{
				current.Start = static_cast<uint32_t>(offset);
			}
			current.End = static_cast<uint32_t>(offset + length);
			current.Codepoints.push_back(static_cast<uint32_t>(cp));

			uint32_t script =
				hb_unicode_script(funcs, static_cast<hb_codepoint_t>(cp));
			if (script != HB_SCRIPT_COMMON && script != HB_SCRIPT_INHERITED &&
				current.Script == 0)
			{
				current.Script = script;
			}
			current.Sticky = current.Script == 0;

			previous = cp;
			offset += static_cast<size_t>(length);
		}

		if (!current.Codepoints.empty())
		{
			out.push_back(std::move(current));
		}
		return out;
	}

	auto resolveScripts(std::vector<Grapheme>& graphemes) -> void
	{
		size_t begin = 0;
		while (begin < graphemes.size())
		{
			size_t end = begin;
			while (end < graphemes.size() && !isSeparator(graphemes[end]))
			{
				end++;
			}
			if (end < graphemes.size())
			{
				end++;
			}

			uint32_t firstStrong = 0;
			for (size_t i = begin; i < end; i++)
			{
				if (!graphemes[i].Sticky)
				{
					firstStrong = graphemes[i].Script;
					break;
				}
			}

			uint32_t previous = 0;
			for (size_t i = begin; i < end; i++)
			{
				if (graphemes[i].Sticky)
				{
					graphemes[i].Script = previous != 0 ? previous
														: firstStrong;
				}
				else
				{
					previous = graphemes[i].Script;
				}
			}

			begin = end;
		}
	}

	auto covers(const Grapheme& grapheme, FontFaceId face) -> bool
	{
		for (uint32_t cp : grapheme.Codepoints)
		{
			if (!isIgnorable(cp) && !FontCoverage::Covers(face, cp))
			{
				return false;
			}
		}
		return true;
	}

	auto isSpaceOnly(const Grapheme& g) -> bool
	{
		return std::ranges::all_of(
			g.Codepoints,
			[](uint32_t cp) -> bool
			{
				return isIgnorable(cp) ||
					   utf8proc_category(cp) == UTF8PROC_CATEGORY_ZS;
			}
		);
	}

	auto chooseFace(
		const Grapheme& grapheme,
		std::span<const FontFaceId> stack,
		FontFaceId previous
	) -> FontFaceId
	{
		if (!hasRequirements(grapheme))
		{
			return previous != SIZE_MAX ? previous : stack.front();
		}

		if (isSpaceOnly(grapheme) && previous != SIZE_MAX &&
			covers(grapheme, previous))
		{
			return previous;
		}

		for (FontFaceId face : stack)
		{
			if (covers(grapheme, face))
			{
				return face;
			}
		}

		return stack.front();
	}

	auto TextItem::Itemize(
		std::string_view content,
		std::span<const FontFaceId> stack
	) -> std::vector<TextItem>
	{
		std::vector<TextItem> items;
		if (stack.empty())
		{
			return items;
		}

		if (content.empty())
		{
			items.push_back(
				TextItem{
					.Start = 0,
					.End = 0,
					.Script = HB_SCRIPT_COMMON,
					.Face = stack.front(),
				}
			);
			return items;
		}

		std::vector<Grapheme> graphemes = splitGraphemes(content);
		resolveScripts(graphemes);

		FontFaceId previousFace = SIZE_MAX;
		bool closed = true;
		std::vector<uint32_t> warnedScripts;

		for (const auto& grapheme : graphemes)
		{
			FontFaceId face = chooseFace(grapheme, stack, previousFace);
			if (hasRequirements(grapheme) && !covers(grapheme, face) &&
				std::ranges::find(warnedScripts, grapheme.Script) ==
					warnedScripts.end())
			{
				uint32_t tag = hb_script_to_iso15924_tag(
					static_cast<hb_script_t>(grapheme.Script)
				);
				warnedScripts.push_back(grapheme.Script);
				mc_debug(
					"no registered face covers script {}{}{}{} at byte {}: {}",
					static_cast<char>((tag >> 24) & 0xFF),
					static_cast<char>((tag >> 16) & 0xFF),
					static_cast<char>((tag >> 8) & 0xFF),
					static_cast<char>(tag & 0xFF),
					grapheme.Start,
					content
						.substr(grapheme.Start, grapheme.End - grapheme.Start)
				);
			}

			if (closed || items.empty() ||
				items.back().Script != grapheme.Script ||
				items.back().Face != face)
			{
				items.push_back(
					TextItem{
						.Start = grapheme.Start,
						.End = grapheme.End,
						.Script = grapheme.Script,
						.Face = face,
					}
				);
			}
			else
			{
				items.back().End = grapheme.End;
			}

			bool separator = isSeparator(grapheme);
			closed = separator;
			previousFace = separator ? SIZE_MAX : face;
		}

		return items;
	}
}

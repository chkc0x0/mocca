#pragma once
#include <cstdint>
#include <string>
#include <string_view>

namespace mocca::mtext
{
	// this doesn't need to be a class but i just want it to
	class Unicode
	{
	public:
		[[nodiscard]] static auto Encode(char32_t cp) -> std::string;

		[[nodiscard]] static auto
		NextGraphemeBoundary(std::string_view text, uint32_t byte) -> uint32_t;
		[[nodiscard]] static auto
		PrevGraphemeBoundary(std::string_view text, uint32_t byte) -> uint32_t;
		[[nodiscard]] static auto
		IsGraphemeBoundary(std::string_view text, uint32_t byte) -> bool;

		[[nodiscard]] static auto
		PrevBackspaceBoundary(std::string_view text, uint32_t byte) -> uint32_t;

		[[nodiscard]] static auto
		NextWordBoundary(std::string_view text, uint32_t byte) -> uint32_t;
		[[nodiscard]] static auto
		PrevWordBoundary(std::string_view text, uint32_t byte) -> uint32_t;

		[[nodiscard]] static auto CountGraphemes(std::string_view text)
			-> uint32_t;

		[[nodiscard]] static auto SanitizeUtf8(std::string_view text)
			-> std::string;
	};
}

#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace mocca::platform
{
	struct ClipboardEntry
	{
	public:
		std::string Mime;
		std::vector<uint8_t> Data;
	};

	class PlatformClipboard
	{
	public:
		virtual ~PlatformClipboard() = default;
		virtual auto Formats() -> std::vector<std::string>
		{
			return {};
		}

		virtual auto Get(std::string_view mime)
			-> std::optional<std::vector<uint8_t>>
		{
			return std::nullopt;
		}

		virtual void Set(const std::vector<ClipboardEntry>& offers)
		{
		}

		auto GetText() -> std::string
		{
			auto val = Get("text/plain");
			if (!val.has_value())
			{
				return "";
			}

			return {val.value().begin(), val.value().end()};
		}

		void SetText(std::string_view str)
		{
			Set({
				{
					.Mime = "text/plain",
					.Data = {str.begin(), str.end()},
				},
			});
		}
	};
}
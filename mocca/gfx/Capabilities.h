#pragma once
#include "gfx/DrawCommands.h"
#include <cstdint>
#include <span>

namespace mocca::gfx
{
	enum class Feature : uint64_t
	{
		None = 0,
		RoundedRects = 1ULL << 1,
		DRRects = 1ULL << 2,
		ClipRect = 1ULL << 3,
		ClipRRect = 1ULL << 4,
		ClipPath = 1ULL << 5,
		ImageSampling = 1ULL << 6,
		ImageTint = 1ULL << 7,
		ImageRounded = 1ULL << 8,
		Text = 1ULL << 9,
		Opacity = 1ULL << 10,
		StrokedPaths = 1ULL << 11,
		Transforms = 1ULL << 12,
	};

	constexpr auto bits(Feature feature) -> uint64_t
	{
		return static_cast<uint64_t>(feature);
	}

	struct Capabilities
	{
	public:
		uint64_t Features = 0;
		uint32_t MaxTextureSize = 0;
		uint32_t MaxPathPoints = 0;
		uint32_t TextureFormats = 0;
	};

	inline constexpr uint64_t MinimumFeatures = bits(Feature::RoundedRects) |
												bits(Feature::DRRects) |
												bits(Feature::ClipRect) |
												bits(Feature::ImageSampling) |
												bits(Feature::ImageRounded) |
												bits(Feature::Text) |
												bits(Feature::Transforms);

	[[nodiscard]] auto drawCmdRequiredFeatures(
		const cmds::DrawCommand& command
	) -> uint64_t;

	[[nodiscard]] inline auto drawCmdMissingFeatures(
		std::span<const cmds::DrawCommand> commands,
		const Capabilities& caps
	) -> uint64_t
	{
		uint64_t required = 0;
		for (const auto& command : commands)
		{
			required |= drawCmdRequiredFeatures(command);
		}

		return required & ~caps.Features;
	}

	namespace detail
	{
		auto reportMissingFeatures(
			std::span<const cmds::DrawCommand> commands,
			const gfx::Capabilities& caps,
			uint64_t& reported
		) -> uint64_t;
	}
}

#include "gfx/Capabilities.h"
#include <type_traits>
#include <variant>

namespace mocca::gfx
{
	auto drawCmdRequiredFeatures(const cmds::DrawCommand& command) -> uint64_t
	{
		return std::visit(
			[](const auto& cmd) -> uint64_t
			{
				using T = std::decay_t<decltype(cmd)>;
				uint64_t features = 0;

				if constexpr (std::is_same_v<T, cmds::DrawRectCmd>)
				{
					if (cmd.Paint.HasStroke())
					{
						features |= bits(Feature::StrokedPaths);
					}
				}
				else if constexpr (std::is_same_v<T, cmds::DrawRRectCmd>)
				{
					if (cmd.Rect.Radii.HasRadius())
					{
						features |= bits(Feature::RoundedRects);
					}
					if (cmd.Paint.HasStroke())
					{
						features |= bits(Feature::StrokedPaths);
					}
				}
				else if constexpr (std::is_same_v<T, cmds::DrawDRRectCmd>)
				{
					features |= bits(Feature::DRRects);
					if (cmd.Paint.HasStroke())
					{
						features |= bits(Feature::StrokedPaths);
					}
				}
				else if constexpr (std::is_same_v<T, cmds::DrawImageCmd>)
				{
					features |= bits(Feature::ImageSampling);
					if (cmd.Dst.Radii.HasRadius())
					{
						features |= bits(Feature::ImageRounded);
					}
					if (cmd.Paint.HasStroke())
					{
						features |= bits(Feature::StrokedPaths);
					}
				}
				else if constexpr (std::is_same_v<T, cmds::DrawTextCmd>)
				{
					features |= bits(Feature::Text);
				}
				else if constexpr (std::is_same_v<T, cmds::ClipRectCmd>)
				{
					features |= bits(Feature::ClipRect);
				}
				else if constexpr (std::is_same_v<T, cmds::ClipRRectCmd>)
				{
					features |= bits(Feature::ClipRRect);
				}

				return features;
			},
			command
		);
	}
}
#include "Style.h"
#include "yoga/YGConfig.h"
#include "yoga/YGNodeStyle.h"

namespace mocca::styles::detail::applying
{
	template <typename PxSetter, typename PctSetter>
	void applyLength(const Length& len, PxSetter setPx, PctSetter setPct)
	{
		if (len.Unit == LengthUnit::Percent)
		{
			setPct(len.Value);
		}
		else
		{
			setPx(len.Value);
		}
	}

	void applyWidth(YGNodeRef ref, SizingValue value)
	{
		std::visit(
			[ref](auto&& arg) -> auto
			{
				using T = std::decay_t<decltype(arg)>;

				if constexpr (std::is_same_v<T, Length>)
				{
					applyLength(
						arg,
						[&](float px) -> void { YGNodeStyleSetWidth(ref, px); },
						[&](float pct) -> void
						{ YGNodeStyleSetWidthPercent(ref, pct); }
					);
				}
				else if constexpr (std::is_same_v<T, AutoTag>)
				{
					YGNodeStyleSetWidthAuto(ref);
				}
				else if constexpr (std::is_same_v<T, MaxContentTag>)
				{
					YGNodeStyleSetWidthMaxContent(ref);
				}
				else if constexpr (std::is_same_v<T, FitContentTag>)
				{
					YGNodeStyleSetWidthFitContent(ref);
				}
				else if constexpr (std::is_same_v<T, StretchTag>)
				{
					YGNodeStyleSetWidthStretch(ref);
				}
			},
			value
		);
	}

	void applyHeight(YGNodeRef ref, SizingValue value)
	{
		std::visit(
			[ref](auto&& arg) -> auto
			{
				using T = std::decay_t<decltype(arg)>;

				if constexpr (std::is_same_v<T, Length>)
				{
					applyLength(
						arg,
						[&](float px) -> void { YGNodeStyleSetHeight(ref, px); },
						[&](float pct) -> void
						{ YGNodeStyleSetHeightPercent(ref, pct); }
					);
				}
				else if constexpr (std::is_same_v<T, AutoTag>)
				{
					YGNodeStyleSetHeightAuto(ref);
				}
				else if constexpr (std::is_same_v<T, MaxContentTag>)
				{
					YGNodeStyleSetHeightMaxContent(ref);
				}
				else if constexpr (std::is_same_v<T, FitContentTag>)
				{
					YGNodeStyleSetHeightFitContent(ref);
				}
				else if constexpr (std::is_same_v<T, StretchTag>)
				{
					YGNodeStyleSetHeightStretch(ref);
				}
			},
			value
		);
	}
}
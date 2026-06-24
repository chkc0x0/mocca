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
						[&](float px) -> void
						{ YGNodeStyleSetHeight(ref, px); },
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

	void applyPadding(YGNodeRef ref, Edges<Length> value)
	{
		if (value.Left.Unit == LengthUnit::Percent)
		{
			YGNodeStyleSetPaddingPercent(ref, YGEdgeLeft, value.Left.Value);
		}
		else
		{
			YGNodeStyleSetPadding(ref, YGEdgeLeft, value.Left.Value);
		}

		if (value.Top.Unit == LengthUnit::Percent)
		{
			YGNodeStyleSetPaddingPercent(ref, YGEdgeTop, value.Top.Value);
		}
		else
		{
			YGNodeStyleSetPadding(ref, YGEdgeTop, value.Top.Value);
		}

		if (value.Right.Unit == LengthUnit::Percent)
		{
			YGNodeStyleSetPaddingPercent(ref, YGEdgeRight, value.Right.Value);
		}
		else
		{
			YGNodeStyleSetPadding(ref, YGEdgeRight, value.Right.Value);
		}

		if (value.Bottom.Unit == LengthUnit::Percent)
		{
			YGNodeStyleSetPaddingPercent(ref, YGEdgeBottom, value.Bottom.Value);
		}
		else
		{
			YGNodeStyleSetPadding(ref, YGEdgeBottom, value.Bottom.Value);
		}
	}

	void applyMargin(YGNodeRef ref, Edges<AutoValue<Length>> value)
	{
		auto applyEdge = [&](YGEdge edge, AutoValue<Length> v) -> void
		{
			std::visit(
				[ref, edge](auto&& arg) -> void
				{
					using T = std::decay_t<decltype(arg)>;

					if constexpr (std::is_same_v<T, Length>)
					{
						if (arg.Unit == LengthUnit::Percent)
						{
							YGNodeStyleSetMarginPercent(ref, edge, arg.Value);
						}
						else
						{
							YGNodeStyleSetMargin(ref, edge, arg.Value);
						}
					}
					else if constexpr (std::is_same_v<T, AutoTag>)
					{
						mc_info("{}", (int)edge);
						YGNodeStyleSetMarginAuto(ref, edge);
					}
				},
				v
			);
		};

		applyEdge(YGEdgeLeft, value.Left);
		applyEdge(YGEdgeTop, value.Top);
		applyEdge(YGEdgeRight, value.Right);
		applyEdge(YGEdgeBottom, value.Bottom);
	}
}
#include "Style.h"
#include "Detail.h"
#include "Logger.h"
#include "yoga/YGConfig.h"
#include "yoga/YGNodeStyle.h"

namespace mocca::styles::detail::applying::layout
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

	void applyMinWidth(YGNodeRef ref, SizingValueNoAuto value)
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
						{ YGNodeStyleSetMinWidth(ref, px); },
						[&](float pct) -> void
						{ YGNodeStyleSetMinWidthPercent(ref, pct); }
					);
				}
				else if constexpr (std::is_same_v<T, MaxContentTag>)
				{
					YGNodeStyleSetMinWidthMaxContent(ref);
				}
				else if constexpr (std::is_same_v<T, FitContentTag>)
				{
					YGNodeStyleSetMinWidthFitContent(ref);
				}
				else if constexpr (std::is_same_v<T, StretchTag>)
				{
					YGNodeStyleSetMinWidthStretch(ref);
				}
			},
			value
		);
	}

	void applyMinHeight(YGNodeRef ref, SizingValueNoAuto value)
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
						{ YGNodeStyleSetMinHeight(ref, px); },
						[&](float pct) -> void
						{ YGNodeStyleSetMinHeightPercent(ref, pct); }
					);
				}
				else if constexpr (std::is_same_v<T, MaxContentTag>)
				{
					YGNodeStyleSetMinHeightMaxContent(ref);
				}
				else if constexpr (std::is_same_v<T, FitContentTag>)
				{
					YGNodeStyleSetMinHeightFitContent(ref);
				}
				else if constexpr (std::is_same_v<T, StretchTag>)
				{
					YGNodeStyleSetMinHeightStretch(ref);
				}
			},
			value
		);
	}

	void applyMaxWidth(YGNodeRef ref, SizingValueNoAuto value)
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
						{ YGNodeStyleSetMaxWidth(ref, px); },
						[&](float pct) -> void
						{ YGNodeStyleSetMaxWidthPercent(ref, pct); }
					);
				}
				else if constexpr (std::is_same_v<T, MaxContentTag>)
				{
					YGNodeStyleSetMaxWidthMaxContent(ref);
				}
				else if constexpr (std::is_same_v<T, FitContentTag>)
				{
					YGNodeStyleSetMaxWidthFitContent(ref);
				}
				else if constexpr (std::is_same_v<T, StretchTag>)
				{
					YGNodeStyleSetMaxWidthStretch(ref);
				}
			},
			value
		);
	}

	void applyMaxHeight(YGNodeRef ref, SizingValueNoAuto value)
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
						{ YGNodeStyleSetMaxHeight(ref, px); },
						[&](float pct) -> void
						{ YGNodeStyleSetMaxHeightPercent(ref, pct); }
					);
				}
				else if constexpr (std::is_same_v<T, MaxContentTag>)
				{
					YGNodeStyleSetMaxHeightMaxContent(ref);
				}
				else if constexpr (std::is_same_v<T, FitContentTag>)
				{
					YGNodeStyleSetMaxHeightFitContent(ref);
				}
				else if constexpr (std::is_same_v<T, StretchTag>)
				{
					YGNodeStyleSetMaxHeightStretch(ref);
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

	void applyAlignContent(YGNodeRef ref, Alignment value)
	{
		YGNodeStyleSetAlignContent(ref, (YGAlign)((int)value + 1));
	}

	void applyAlignItems(YGNodeRef ref, Alignment value)
	{
		if (value >= Alignment::SpaceBetween && value < Alignment::Start)
		{
			mc_error(
				ErrorCode::InvalidArgument,
				"space-* cannot be used for AlignItems, falling back to Initial"
			);
			value = Alignment::Stretch;
		}
		YGNodeStyleSetAlignItems(ref, (YGAlign)((int)value + 1));
	}

	void applyAlignSelf(YGNodeRef ref, AutoValue<Alignment> value)
	{
		if (value.index() != 0)
		{
			YGNodeStyleSetAlignSelf(ref, YGAlignAuto);
		}
		else
		{
			if (std::get<Alignment>(value) >= Alignment::SpaceBetween)
			{
				mc_error(
					ErrorCode::InvalidArgument,
					"space-* cannot be used for AlignSelf, falling back to "
					"Initial"
				);
				YGNodeStyleSetAlignSelf(ref, YGAlignAuto);
				return;
			}

			YGNodeStyleSetAlignSelf(
				ref,
				(YGAlign)((int)std::get<Alignment>(value) + 1)
			);
		}
	}

	void applyAspectRatio(YGNodeRef ref, float value)
	{
		YGNodeStyleSetAspectRatio(ref, value);
	}

	void applyDisplay(YGNodeRef ref, DisplayType value)
	{
		YGNodeStyleSetDisplay(ref, (YGDisplay)((int)value));
	}

	void applyBorder(YGNode* ref, Edges<Length> value)
	{
		if (value.Left.Unit == LengthUnit::Percent ||
			value.Top.Unit == LengthUnit::Percent ||
			value.Right.Unit == LengthUnit::Percent ||
			value.Bottom.Unit == LengthUnit::Percent)
		{
			mc_info("percentages are not allowed in Border");
			return;
		}

		YGNodeStyleSetBorder(ref, YGEdgeLeft, value.Left.Value);
		YGNodeStyleSetBorder(ref, YGEdgeTop, value.Top.Value);
		YGNodeStyleSetBorder(ref, YGEdgeRight, value.Right.Value);
		YGNodeStyleSetBorder(ref, YGEdgeBottom, value.Bottom.Value);
	}

	void applyFlexBasis(YGNodeRef ref, SizingValue value)
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
						{ YGNodeStyleSetFlexBasis(ref, px); },
						[&](float pct) -> void
						{ YGNodeStyleSetFlexBasisPercent(ref, pct); }
					);
				}
				else if constexpr (std::is_same_v<T, AutoTag>)
				{
					YGNodeStyleSetFlexBasisAuto(ref);
				}
				else if constexpr (std::is_same_v<T, MaxContentTag>)
				{
					YGNodeStyleSetFlexBasisMaxContent(ref);
				}
				else if constexpr (std::is_same_v<T, FitContentTag>)
				{
					YGNodeStyleSetFlexBasisFitContent(ref);
				}
				else if constexpr (std::is_same_v<T, StretchTag>)
				{
					YGNodeStyleSetFlexBasisStretch(ref);
				}
			},
			value
		);
	}

	void applyFlexGrow(YGNodeRef ref, float value)
	{
		YGNodeStyleSetFlexGrow(ref, value);
	}

	void applyFlexShrink(YGNodeRef ref, float value)
	{
		YGNodeStyleSetFlexShrink(ref, value);
	}

	void applyFlex(YGNodeRef ref, float value)
	{
		YGNodeStyleSetFlex(ref, value);
	}

	void applyFlexDirection(YGNodeRef ref, FlexDirections value)
	{
		YGNodeStyleSetFlexDirection(ref, (YGFlexDirection)((int)value));
	}

	void applyFlexWrap(YGNodeRef ref, WrappingType value)
	{
		YGNodeStyleSetFlexWrap(ref, (YGWrap)((int)value));
	}

	void applyGap(YGNodeRef ref, Axes<Length> value)
	{
		if (value.Horizontal.Unit == LengthUnit::Percent)
		{
			YGNodeStyleSetGapPercent(ref, YGGutterRow, value.Horizontal.Value);
		}
		else
		{
			YGNodeStyleSetGap(ref, YGGutterRow, value.Horizontal.Value);
		}

		if (value.Vertical.Unit == LengthUnit::Percent)
		{
			YGNodeStyleSetGapPercent(ref, YGGutterColumn, value.Vertical.Value);
		}
		else
		{
			YGNodeStyleSetGap(ref, YGGutterColumn, value.Vertical.Value);
		}
	}

	void applyPosition(YGNodeRef ref, Edges<AutoValue<Length>> value)
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
							YGNodeStyleSetPositionPercent(ref, edge, arg.Value);
						}
						else
						{
							YGNodeStyleSetPosition(ref, edge, arg.Value);
						}
					}
					else if constexpr (std::is_same_v<T, AutoTag>)
					{
						YGNodeStyleSetPositionAuto(ref, edge);
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

	void applyJustifyContent(YGNodeRef ref, Justification value)
	{
		YGNodeStyleSetJustifyContent(ref, (YGJustify)((int)value + 1));
	}

	void applyJustifyItems(YGNodeRef ref, Justification value)
	{
		YGNodeStyleSetJustifyItems(ref, (YGJustify)((int)value + 1));
	}

	void applyJustifySelf(YGNodeRef ref, AutoValue<Justification> value)
	{
		if (value.index() != 0)
		{
			YGNodeStyleSetJustifySelf(ref, YGJustifyAuto);
		}
		else
		{
			YGNodeStyleSetJustifySelf(
				ref,
				(YGJustify)((int)std::get<Justification>(value) + 1)
			);
		}
	}

	void applyLayoutDirection(YGNodeRef ref, LayoutDirections value)
	{
		YGNodeStyleSetDirection(ref, (YGDirection)((int)value + 1));
	}

	void applyOverflow(YGNodeRef ref, OverflowType value)
	{
		YGNodeStyleSetOverflow(ref, (YGOverflow)((int)value));
	}
}

namespace mocca::styles::detail::applying::render
{
	void applyBackgroundColor(mocca::detail::Node* ref, Color value)
	{
		ref->Style.BackgroundColor = value;
	}

	void applyTextColor(mocca::detail::Node* ref, Color value)
	{
		ref->Style.TextColor = value;
	}
}
#include "style/StyleApply.h"
#include "core/Logger.h"
#include "yoga/YGConfig.h"
#include "yoga/YGNodeStyle.h"
#include <cmath>
#include <limits>

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
				else
				{
					uint8_t v = arg;

					switch (v)
					{
					case Auto:
						YGNodeStyleSetWidthAuto(ref);
						break;
					case MaxContent:
						YGNodeStyleSetWidthMaxContent(ref);
						break;
					case FitContent:
						YGNodeStyleSetWidthFitContent(ref);
						break;
					case Stretch:
						YGNodeStyleSetWidthStretch(ref);
						break;
					default:
						mc_error(
							ErrorCode::InvalidArgument,
							"Width does not take...whatever this is ({})",
							arg
						);
						break;
					}
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
				else
				{
					uint8_t v = arg;

					switch (v)
					{
					case Auto:
						YGNodeStyleSetHeightAuto(ref);
						break;
					case MaxContent:
						YGNodeStyleSetHeightMaxContent(ref);
						break;
					case FitContent:
						YGNodeStyleSetHeightFitContent(ref);
						break;
					case Stretch:
						YGNodeStyleSetHeightStretch(ref);
						break;
					default:
						mc_error(
							ErrorCode::InvalidArgument,
							"Height does not take...whatever this is ({})",
							arg
						);
						break;
					}
				}
			},
			value
		);
	}

	void applyMinWidth(YGNodeRef ref, SizingValue value)
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
				else
				{
					uint8_t v = arg;

					switch (v)
					{
					case Auto:
						YGNodeStyleSetMinWidth(ref, 0);
						mc_error(
							ErrorCode::InvalidArgument,
							"MinWidth cannot take Auto, defaulting to 0"
						);
						break;
					case MaxContent:
						YGNodeStyleSetMinWidthMaxContent(ref);
						break;
					case FitContent:
						YGNodeStyleSetMinWidthFitContent(ref);
						break;
					case Stretch:
						YGNodeStyleSetMinWidthStretch(ref);
						break;
					default:
						mc_error(
							ErrorCode::InvalidArgument,
							"MinWidth does not take...whatever this is ({})",
							arg
						);
						break;
					}
				}
			},
			value
		);
	}

	void applyMinHeight(YGNodeRef ref, SizingValue value)
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
				else
				{
					uint8_t v = arg;

					switch (v)
					{
					case Auto:
						YGNodeStyleSetMinHeight(ref, 0);
						mc_error(
							ErrorCode::InvalidArgument,
							"MinHeight cannot take Auto, defaulting to 0"
						);
						break;
					case MaxContent:
						YGNodeStyleSetMinHeightMaxContent(ref);
						break;
					case FitContent:
						YGNodeStyleSetMinHeightFitContent(ref);
						break;
					case Stretch:
						YGNodeStyleSetMinHeightStretch(ref);
						break;
					default:
						mc_error(
							ErrorCode::InvalidArgument,
							"MinHeight does not take...whatever this is ({})",
							arg
						);
						break;
					}
				}
			},
			value
		);
	}

	void applyMaxWidth(YGNodeRef ref, SizingValue value)
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
				else
				{
					uint8_t v = arg;

					switch (v)
					{
					case Auto:
						YGNodeStyleSetMaxWidth(ref, 0);
						mc_error(
							ErrorCode::InvalidArgument,
							"MaxWidth cannot take Auto, defaulting to 0"
						);
						break;
					case MaxContent:
						YGNodeStyleSetMaxWidthMaxContent(ref);
						break;
					case FitContent:
						YGNodeStyleSetMaxWidthFitContent(ref);
						break;
					case Stretch:
						YGNodeStyleSetMaxWidthStretch(ref);
						break;
					default:
						mc_error(
							ErrorCode::InvalidArgument,
							"MaxWidth does not take...whatever this is ({})",
							arg
						);
						break;
					}
				}
			},
			value
		);
	}

	void applyMaxHeight(YGNodeRef ref, SizingValue value)
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
				else
				{
					uint8_t v = arg;

					switch (v)
					{
					case Auto:
						YGNodeStyleSetMaxHeight(ref, 0);
						mc_error(
							ErrorCode::InvalidArgument,
							"MaxHeight cannot take Auto, defaulting to 0"
						);
						break;
					case MaxContent:
						YGNodeStyleSetMaxHeightMaxContent(ref);
						break;
					case FitContent:
						YGNodeStyleSetMaxHeightFitContent(ref);
						break;
					case Stretch:
						YGNodeStyleSetMaxHeightStretch(ref);
						break;
					default:
						mc_error(
							ErrorCode::InvalidArgument,
							"MaxHeight does not take...whatever this is ({})",
							arg
						);
						break;
					}
				}
			},
			value
		);
	}

	void applyPadding(YGNodeRef ref, Length value, YGEdge edge)
	{
		if (value.Unit == LengthUnit::Percent)
		{
			YGNodeStyleSetPaddingPercent(ref, edge, value.Value);
		}
		else
		{
			YGNodeStyleSetPadding(ref, edge, value.Value);
		}
	}

	void applyPaddingTop(YGNodeRef ref, Length value)
	{
		applyPadding(ref, value, YGEdgeTop);
	}

	void applyPaddingRight(YGNodeRef ref, Length value)
	{
		applyPadding(ref, value, YGEdgeRight);
	}

	void applyPaddingBottom(YGNodeRef ref, Length value)
	{
		applyPadding(ref, value, YGEdgeBottom);
	}

	void applyPaddingLeft(YGNodeRef ref, Length value)
	{
		applyPadding(ref, value, YGEdgeLeft);
	}

	void applyMargin(YGNodeRef ref, EnumValue<Length> v, YGEdge edge)
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
				else if (arg == Auto)
				{
					YGNodeStyleSetMarginAuto(ref, edge);
				}
			},
			v
		);
	}

	void applyMarginTop(YGNodeRef ref, EnumValue<Length> value)
	{
		applyMargin(ref, value, YGEdgeTop);
	}

	void applyMarginRight(YGNodeRef ref, EnumValue<Length> value)
	{
		applyMargin(ref, value, YGEdgeRight);
	}

	void applyMarginBottom(YGNodeRef ref, EnumValue<Length> value)
	{
		applyMargin(ref, value, YGEdgeBottom);
	}

	void applyMarginLeft(YGNodeRef ref, EnumValue<Length> value)
	{
		applyMargin(ref, value, YGEdgeLeft);
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

	void applyAlignSelf(YGNodeRef ref, EnumValue<Alignment> value)
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
		bool autoRatio = value == 0.0F || std::isinf(value);
		YGNodeStyleSetAspectRatio(
			ref,
			autoRatio ? std::numeric_limits<float>::quiet_NaN() : value
		);
	}

	void applyDisplay(YGNodeRef ref, DisplayType value)
	{
		YGNodeStyleSetDisplay(ref, (YGDisplay)((int)value));
	}

	void applyBorder(YGNode* ref, Length value, YGEdge edge)
	{
		if (value.Unit == LengthUnit::Percent)
		{
			mc_info("percentages are not allowed in Border");
			return;
		}

		YGNodeStyleSetBorder(ref, edge, value.Value);
	}

	void applyBorderTop(YGNodeRef ref, Length value)
	{
		applyBorder(ref, value, YGEdgeTop);
	}

	void applyBorderRight(YGNodeRef ref, Length value)
	{
		applyBorder(ref, value, YGEdgeRight);
	}

	void applyBorderBottom(YGNodeRef ref, Length value)
	{
		applyBorder(ref, value, YGEdgeBottom);
	}

	void applyBorderLeft(YGNodeRef ref, Length value)
	{
		applyBorder(ref, value, YGEdgeLeft);
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
				else
				{
					uint8_t v = arg;

					switch (v)
					{
					case Auto:
						YGNodeStyleSetFlexBasisAuto(ref);
						break;
					case MaxContent:
						YGNodeStyleSetFlexBasisMaxContent(ref);
						break;
					case FitContent:
						YGNodeStyleSetFlexBasisFitContent(ref);
						break;
					case Stretch:
						YGNodeStyleSetFlexBasisStretch(ref);
						break;
					default:
						mc_error(
							ErrorCode::InvalidArgument,
							"FlexBasis does not take...whatever this is ({})",
							arg
						);
						break;
					}
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

	void applyGap(YGNodeRef ref, Length value, YGGutter axis)
	{
		if (value.Unit == LengthUnit::Percent)
		{
			YGNodeStyleSetGapPercent(ref, axis, value.Value);
		}
		else
		{
			YGNodeStyleSetGap(ref, axis, value.Value);
		}
	}

	void applyGapHorizontal(YGNodeRef ref, Length value)
	{
		applyGap(ref, value, YGGutterColumn);
	}

	void applyGapVertical(YGNodeRef ref, Length value)
	{
		applyGap(ref, value, YGGutterRow);
	}

	void applyPositionEdge(YGNodeRef ref, EnumValue<Length> v, YGEdge edge)
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
				else if (arg == 0)
				{
					YGNodeStyleSetPositionAuto(ref, edge);
				}
			},
			v
		);
	}

	void applyTop(YGNodeRef ref, EnumValue<Length> value)
	{
		applyPositionEdge(ref, value, YGEdgeTop);
	}

	void applyLeft(YGNodeRef ref, EnumValue<Length> value)
	{
		applyPositionEdge(ref, value, YGEdgeLeft);
	}

	void applyRight(YGNodeRef ref, EnumValue<Length> value)
	{
		applyPositionEdge(ref, value, YGEdgeRight);
	}

	void applyBottom(YGNodeRef ref, EnumValue<Length> value)
	{
		applyPositionEdge(ref, value, YGEdgeBottom);
	}

	void applyJustifyContent(YGNodeRef ref, Justification value)
	{
		YGNodeStyleSetJustifyContent(ref, (YGJustify)((int)value + 1));
	}

	void applyJustifyItems(YGNodeRef ref, Justification value)
	{
		YGNodeStyleSetJustifyItems(ref, (YGJustify)((int)value + 1));
	}

	void applyJustifySelf(YGNodeRef ref, EnumValue<Justification> value)
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

	void applyPosition(YGNodeRef ref, PositionType value)
	{
		YGNodeStyleSetPositionType(ref, (YGPositionType)value);
	}
}
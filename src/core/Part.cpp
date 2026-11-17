#include "core/Part.h"
#include <algorithm>

namespace mocca
{
	auto PartAxis::Resolve(
		const std::optional<styles::Length>& start,
		const std::optional<styles::Length>& end,
		const std::optional<styles::Length>& size,
		float refPos,
		float refSize,
		float intrinsic,
		float emBasis,
		const styles::EnumValue<styles::Length>& marginStart,
		const styles::EnumValue<styles::Length>& marginEnd,
		bool allowOvershoot
	) -> PartAxis
	{
		bool hasStart = start.has_value();
		bool hasEnd = end.has_value();

		auto startMargin = detail::resolvedLength(
			detail::specifiedLength(marginStart),
			refSize,
			emBasis
		);
		auto endMargin = detail::resolvedLength(
			detail::specifiedLength(marginEnd),
			refSize,
			emBasis
		);

		bool autoStart = !startMargin.has_value();
		bool autoEnd = !endMargin.has_value();

		float marginStartPx = startMargin.value_or(0.0F);
		float marginEndPx = endMargin.value_or(0.0F);

		float s = hasStart ? detail::resolveLength(*start, refSize, emBasis)
						   : 0.0F;
		float e = hasEnd ? detail::resolveLength(*end, refSize, emBasis) : 0.0F;

		bool hasMargin = autoStart || autoEnd || marginStartPx != 0.0F ||
						 marginEndPx != 0.0F;
		float available = refSize - marginStartPx - marginEndPx;

		float w = intrinsic;
		if (size.has_value())
		{
			w = detail::resolveLength(*size, refSize, emBasis);
		}
		else if (hasStart && hasEnd)
		{
			w = available - s - e;
		}

		if (hasMargin)
		{
			w = std::min(w, available);
		}

		w = std::max(w, 0.0F);

		if (autoStart || autoEnd)
		{
			float insets = (hasStart ? s : 0.0F) + (hasEnd ? e : 0.0F);
			float free = std::max(
				0.0F,
				refSize - marginStartPx - marginEndPx - insets - w
			);

			if (autoStart && autoEnd)
			{
				marginStartPx = free * 0.5F;
				marginEndPx = free * 0.5F;
			}
			else if (autoStart)
			{
				marginStartPx = free;
			}
			else
			{
				marginEndPx = free;
			}
		}

		float pos = refPos + marginStartPx;
		if (hasStart)
		{
			pos = refPos + marginStartPx + s;
		}
		else if (hasEnd)
		{
			pos = refPos + refSize - marginEndPx - e - w;
		}

		if (hasMargin)
		{
			float low = refPos + (allowOvershoot ? 0.0F : marginStartPx);
			float high = refPos + refSize -
						 (allowOvershoot ? 0.0F : marginEndPx) - w;
			pos = high >= low ? std::clamp(pos, low, high) : low;
		}

		return {.Pos = pos, .Size = w};
	}
}
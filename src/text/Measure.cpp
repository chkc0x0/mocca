#include "text/Measure.h"
#include "core/Element.h"
#include "core/Node.h"
#include "text/TextLayoutBuilder.h"
#include <cmath>
#include <string>

namespace mocca::mtext::detail
{
	auto measureFunc(
		const YGNode* ref,
		float width,
		YGMeasureMode widthMode,
		float height,
		YGMeasureMode heightMode
	) -> YGSize
	{
		auto* self = static_cast<mocca::detail::Node*>(YGNodeGetContext(ref));
		if ((self == nullptr) || !self->IsText())
		{
			return YGSize{.width = 0, .height = 0};
		}

		const std::string& content = std::get<TextElement>(self->Kind).Content;

		bool wrap = widthMode != YGMeasureModeUndefined;
		float maxWidth = wrap ? width : INFINITY;

		auto layout = TextLayoutBuilder::ResolveTextLayout(
			self->Style,
			content,
			maxWidth,
			wrap
		);

		float resolvedWidth = widthMode == YGMeasureModeExactly
								  ? width
								  : std::ceil(layout->Size.X);
		float resolvedHeight = heightMode == YGMeasureModeExactly
								   ? height
								   : std::ceil(layout->Size.Y);

		if (widthMode == YGMeasureModeAtMost)
		{
			resolvedWidth = std::fmin(resolvedWidth, width);
		}
		if (heightMode == YGMeasureModeAtMost)
		{
			resolvedHeight = std::fmin(resolvedHeight, height);
		}

		return YGSize{.width = resolvedWidth, .height = resolvedHeight};
	}
}

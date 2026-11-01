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

		const auto& textElement = std::get<TextElement>(self->Kind);
		const std::string& content = textElement.Content;

		float caretWidth = textElement.Caret
							   ? std::max(0.0F, textElement.Caret->Width)
							   : 0.0F;

		bool wrap = widthMode != YGMeasureModeUndefined;
		float maxWidth = wrap ? std::max(0.0F, width - caretWidth) : INFINITY;

		auto layout = TextLayoutBuilder::ResolveTextLayout(
			self->Style,
			content,
			maxWidth,
			wrap
		);

		float resolvedWidth = widthMode == YGMeasureModeExactly
								  ? width
								  : std::ceil(layout->Size.X) + caretWidth;

		float naturalHeight = layout->Lines.empty()
								  ? std::ceil(layout->LineHeight)
								  : std::ceil(layout->Size.Y);
		float resolvedHeight = heightMode == YGMeasureModeExactly
								   ? height
								   : naturalHeight;

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

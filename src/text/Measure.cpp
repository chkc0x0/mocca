#include "text/Measure.h"
#include "core/Element.h"
#include "core/Logger.h"
#include "core/Node.h"
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

		// TODO: read font size from computed style once styling exists. Hardcoded now.
		constexpr int fontSize = 16;

		auto textWidth = static_cast<float>(content.size() * 8);
		mc_debug("{}", content.size());

		return YGSize{
			.width = textWidth,
			.height = static_cast<float>(fontSize),
		};
	}
}
#pragma once
#include "Identifiers.h"
#include "utils/Math.h"
#include <memory>

namespace mocca
{
	class Surface;
	struct Element;

	namespace detail
	{
		struct Node;
	}

	namespace mtext
	{
		struct TextPosition;
		struct TextLayout;
	}

	class NodeRef
	{
	public:
		[[nodiscard]] auto Valid() const -> bool;
		[[nodiscard]] auto Id() const -> detail::NodeId;
		[[nodiscard]] auto Surface() -> class Surface*;

		[[nodiscard]] auto Rect() const -> Rectangle;
		[[nodiscard]] auto LayoutRect() const -> Rectangle;

		void ScrollIntoView();
		void ScrollRectIntoView(Rectangle target);

		void Focus();
		void Blur();
		[[nodiscard]] auto IsFocused() const -> bool;

		[[nodiscard]] auto PositionAtPoint(Vector2 surfacePoint) const
			-> mtext::TextPosition;
		[[nodiscard]] auto CaretRect(mtext::TextPosition position) const
			-> Rectangle;
		[[nodiscard]] auto TextLayout() const
			-> std::shared_ptr<const mtext::TextLayout>;

	private:
		detail::NodeId _id = 0;
		class Surface* _surface = nullptr;
		detail::Node* _node = nullptr;
		friend struct detail::Node;
		friend struct Element;
	};
}

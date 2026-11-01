#pragma once
#include "core/ElementEvents.h"
#include "Identifiers.h"
#include "utils/Math.h"
#include <memory>
#include <typeinfo>

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

		template <class E> void Emit(EventKey<E> key, E& event) const
		{
			_emitImpl(key.Id, typeid(E), &event);
		}

		template <class E> void Emit(CustomEvent<E> ev, E& event) const
		{
			Emit(CustomEvent<E>::Key(), event);
		}

		[[nodiscard]] auto PositionAtPoint(Vector2 surfacePoint) const
			-> mtext::TextPosition;
		[[nodiscard]] auto CaretRect(mtext::TextPosition position) const
			-> Rectangle;
		[[nodiscard]] auto TextLayout() const
			-> std::shared_ptr<const mtext::TextLayout>;

	private:
		void _emitImpl(uint32_t id, const std::type_info& type, void* event) const;

		detail::NodeId _id = 0;
		class Surface* _surface = nullptr;
		detail::Node* _node = nullptr;
		friend struct detail::Node;
		friend struct Element;
	};
}

#include "core/NodeRef.h"
#include "core/Node.h"
#include "core/Surface.h"
#include "input/Scroll.h"
#include "style/ComputedStyle.h"
#include "text/TextGeometry.h"
#include "text/TextLayout.h"
#include "text/TextLayoutBuilder.h"
#include "yoga/YGNodeLayout.h"
#include <algorithm>

namespace mocca
{
	auto findNode(detail::NodeId id, Surface* surface) -> detail::Node*
	{
		if (surface == nullptr || id == 0)
		{
			return nullptr;
		}

		return detail::Node::FindNodeById(surface->Tree(), id);
	}

	auto subtreeContainsId(const detail::Node* node, detail::NodeId id) -> bool
	{
		if (node == nullptr)
		{
			return false;
		}

		if (node->Id == id)
		{
			return true;
		}

		for (const auto& child : node->Children)
		{
			if (subtreeContainsId(child.get(), id))
			{
				return true;
			}
		}

		return false;
	}

	auto textLayoutFor(const detail::Node& node)
		-> std::shared_ptr<const mtext::TextLayout>
	{
		const auto& text = std::get<TextElement>(node.Kind);
		float caretWidth = text.Caret ? std::max(0.0F, text.Caret->Width)
									  : 0.0F;
		float textWidth =
			std::max(0.0F, YGNodeLayoutGetWidth(node.YogaNode) - caretWidth);
		return mtext::detail::TextLayoutBuilder::ResolveTextLayout(
			node.Style,
			text.Content,
			textWidth,
			true
		);
	}

	auto NodeRef::Valid() const -> bool
	{
		return findNode(_id, _surface) != nullptr;
	}

	auto NodeRef::Id() const -> detail::NodeId
	{
		return _id;
	}

	auto NodeRef::Surface() -> class Surface*
	{
		return _surface;
	}

	auto NodeRef::Rect() const -> Rectangle
	{
		auto* node = findNode(_id, _surface);
		if (node == nullptr)
		{
			return {};
		}

		return detail::paintedRect(*node);
	}

	auto NodeRef::LayoutRect() const -> Rectangle
	{
		auto* node = findNode(_id, _surface);
		if (node == nullptr)
		{
			return {};
		}

		return {
			.X = node->GetX(),
			.Y = node->GetY(),
			.Width = YGNodeLayoutGetWidth(node->YogaNode),
			.Height = YGNodeLayoutGetHeight(node->YogaNode),
		};
	}

	void NodeRef::Focus()
	{
		if (_surface != nullptr)
		{
			_surface->FocusNode(_id, FocusReason::Programmatic);
		}
	}

	void NodeRef::Blur()
	{
		if (IsFocused() && _surface != nullptr)
		{
			_surface->ClearFocus();
		}
	}

	auto NodeRef::IsFocused() const -> bool
	{
		if (_surface == nullptr)
		{
			return false;
		}

		auto focusedId = _surface->GetFocusedNode();
		if (focusedId == 0)
		{
			return false;
		}

		if (_surface->Tree() == nullptr)
		{
			return subtreeContainsId(_node, focusedId);
		}

		auto* focused = findNode(focusedId, _surface);

		for (auto* n = focused; n != nullptr; n = n->Parent)
		{
			if (n->Id == _id)
			{
				return true;
			}
		}

		return false;
	}

	auto NodeRef::IsFocusVisible() const -> bool
	{
		if (!IsFocused() || _surface == nullptr)
		{
			return false;
		}

		return _surface->GetLastFocusReason() != FocusReason::Pointer;
	}

	void NodeRef::_emitImpl(
		uint32_t id,
		const std::type_info& type,
		void* event
	) const
	{
		auto* node = findNode(_id, _surface);
		if (node != nullptr && node->Events != nullptr)
		{
			node->Events->_callRaw(id, type, event);
		}
	}

	void NodeRef::ScrollIntoView()
	{
		ScrollRectIntoView(Rect());
	}

	void NodeRef::ScrollRectIntoView(Rectangle target)
	{
		auto* node = findNode(_id, _surface);
		if (node == nullptr)
		{
			return;
		}

		detail::scrollRectIntoView(node, _surface, target);
	}

	auto NodeRef::PositionAtPoint(Vector2 surfacePoint) const
		-> mtext::TextPosition
	{
		auto* node = findNode(_id, _surface);
		if (node == nullptr || !node->IsText())
		{
			return {};
		}

		auto layout = textLayoutFor(*node);
		Rectangle rect = detail::paintedRect(*node);
		const auto& text = std::get<TextElement>(node->Kind);
		float caretWidth = text.Caret ? std::max(0.0F, text.Caret->Width)
									  : 0.0F;
		float textWidth = std::max(0.0F, rect.Width - caretWidth);

		return mtext::TextGeometry::PositionAtPoint(
			*layout,
			{.X = surfacePoint.X - rect.X, .Y = surfacePoint.Y - rect.Y},
			textWidth,
			node->Style.Get<styles::StyleProperty::TextAlign>()
		);
	}

	auto NodeRef::CaretRect(mtext::TextPosition position) const -> Rectangle
	{
		auto* node = findNode(_id, _surface);
		if (node == nullptr || !node->IsText())
		{
			return {};
		}

		auto layout = textLayoutFor(*node);
		Rectangle rect = detail::paintedRect(*node);
		const auto& text = std::get<TextElement>(node->Kind);
		float caretWidth = text.Caret ? std::max(0.0F, text.Caret->Width)
									  : 1.0F;
		float textWidth = std::max(
			0.0F,
			rect.Width - (text.Caret ? std::max(0.0F, text.Caret->Width) : 0.0F)
		);
		auto caret = mtext::TextGeometry::CaretAt(
			*layout,
			position,
			textWidth,
			node->Style.Get<styles::StyleProperty::TextAlign>()
		);

		return Rectangle{
			.X = std::round(rect.X + caret.X),
			.Y = std::round(rect.Y + caret.Y),
			.Width = std::max(1.0F, std::round(caretWidth)),
			.Height = std::round(caret.Height),
		};
	}

	auto NodeRef::TextLayout() const -> std::shared_ptr<const mtext::TextLayout>
	{
		auto* node = findNode(_id, _surface);
		if (node == nullptr || !node->IsText())
		{
			return nullptr;
		}

		return textLayoutFor(*node);
	}
}

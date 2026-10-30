#include "text/TextEditor.h"
#include "text/Unicode.h"
#include "utf8proc.h"
#include <algorithm>
#include <cstddef>
#include <utility>

namespace mocca::mtext
{
	constexpr size_t MaxUndoEntries = 128;

	auto isWordCategory(utf8proc_category_t category) -> bool
	{
		return (category >= UTF8PROC_CATEGORY_LU &&
				category <= UTF8PROC_CATEGORY_LO) ||
			   (category >= UTF8PROC_CATEGORY_MN &&
				category <= UTF8PROC_CATEGORY_ME) ||
			   (category >= UTF8PROC_CATEGORY_ND &&
				category <= UTF8PROC_CATEGORY_NO) ||
			   category == UTF8PROC_CATEGORY_PC;
	}

	auto segmentContainsWord(std::string_view text) -> bool
	{
		size_t offset = 0;
		while (offset < text.size())
		{
			utf8proc_int32_t codepoint = 0;
			auto length = utf8proc_iterate(
				reinterpret_cast<const utf8proc_uint8_t*>(text.data() + offset),
				static_cast<utf8proc_ssize_t>(text.size() - offset),
				&codepoint
			);
			if (length <= 0)
			{
				offset++;
				continue;
			}
			if (isWordCategory(utf8proc_category(codepoint)))
			{
				return true;
			}
			offset += static_cast<size_t>(length);
		}
		return false;
	}

	auto isSoftWrapBoundary(const TextLayout& layout, uint32_t offset) -> bool
	{
		for (size_t i = 1; i < layout.Lines.size(); i++)
		{
			if (layout.Lines[i].Start == offset &&
				layout.Lines[i - 1].End == offset)
			{
				return true;
			}
		}
		return false;
	}

	auto segmentIsWord(std::string_view text, uint32_t start, uint32_t end)
		-> bool
	{
		if (start >= end || end > text.size())
		{
			return false;
		}
		return segmentContainsWord(text.substr(start, end - start));
	}

	auto TextEditor::_sanitize(std::string_view text) const -> std::string
	{
		std::string result = Unicode::SanitizeUtf8(text);
		if (!Multiline)
		{
			result.erase(
				std::remove_if(
					result.begin(),
					result.end(),
					[](char c) -> bool { return c == '\r' || c == '\n'; }
				),
				result.end()
			);
		}
		return result;
	}

	auto TextEditor::_snapOffset(uint32_t offset) const -> uint32_t
	{
		auto size = static_cast<uint32_t>(_text.size());
		uint32_t clamped = std::min(offset, size);
		if (clamped == 0 || clamped == size)
		{
			return clamped;
		}

		uint32_t start = Unicode::PrevGraphemeBoundary(_text, clamped);
		return Unicode::NextGraphemeBoundary(_text, start) == clamped ? clamped
																	  : start;
	}

	void TextEditor::_clampSelection()
	{
		auto size = static_cast<uint32_t>(_text.size());

		_selection.Anchor = _snapOffset(std::min(_selection.Anchor, size));

		uint32_t rawActive = std::min(_selection.Active, size);
		uint32_t active = _snapOffset(rawActive);
		if (active != rawActive)
		{
			_selection.ActiveAffinity = CaretAffinity::Downstream;
		}
		_selection.Active = active;
	}

	auto TextEditor::_makeSnapshot() const -> Snapshot
	{
		return Snapshot{.Text = _text, .Selection = _selection};
	}

	void TextEditor::_pushUndo(EditKind kind)
	{
		if (_coalescing && _lastEditKind == kind)
		{
			return;
		}

		_undo.push_back(_makeSnapshot());
		if (_undo.size() > MaxUndoEntries)
		{
			_undo.erase(_undo.begin());
		}
		_redo.clear();
		_lastEditKind = kind;
		_coalescing = true;
	}

	void TextEditor::BreakUndoCoalescing()
	{
		_coalescing = false;
		_lastEditKind = EditKind::None;
	}

	void TextEditor::_applyCaret(
		TextPosition position,
		bool extend,
		bool keepPreferred
	)
	{
		uint32_t offset = _snapOffset(position.Offset);
		CaretAffinity affinity = position.Affinity;
		if (offset != position.Offset)
		{
			affinity = CaretAffinity::Downstream;
		}

		if (extend)
		{
			_selection.Active = offset;
			_selection.ActiveAffinity = affinity;
		}
		else
		{
			_selection.Anchor = offset;
			_selection.Active = offset;
			_selection.ActiveAffinity = affinity;
		}

		if (!keepPreferred)
		{
			_preferredX.reset();
		}
	}

	void TextEditor::SetText(const std::string& text, bool resetUndo)
	{
		CancelPreedit();
		_text = _sanitize(text);
		_clampSelection();
		BreakUndoCoalescing();
		_preferredX.reset();

		if (resetUndo)
		{
			_undo.clear();
			_redo.clear();
		}
	}

	auto TextEditor::Text() const -> const std::string&
	{
		return _text;
	}

	auto TextEditor::Selection() const -> const TextSelection&
	{
		return _selection;
	}

	auto TextEditor::Caret() const -> TextPosition
	{
		return TextPosition{
			.Offset = _selection.Active,
			.Affinity = _selection.ActiveAffinity,
		};
	}

	void TextEditor::SetCaret(TextPosition position, bool extend)
	{
		CancelPreedit();
		_applyCaret(position, extend, false);
		BreakUndoCoalescing();
	}

	void TextEditor::SetSelection(TextSelection selection)
	{
		CancelPreedit();
		_selection = selection;
		_clampSelection();
		BreakUndoCoalescing();
		_preferredX.reset();
	}

	void TextEditor::SelectAll()
	{
		CancelPreedit();
		auto size = static_cast<uint32_t>(_text.size());
		_selection = TextSelection{
			.Anchor = 0,
			.Active = size,
			.ActiveAffinity = CaretAffinity::Upstream,
		};
		BreakUndoCoalescing();
		_preferredX.reset();
	}

	auto TextEditor::SelectedText() const -> std::string
	{
		uint32_t start = _selection.Start();
		return _text.substr(start, _selection.End() - start);
	}

	void TextEditor::_insertSanitized(std::string_view text, EditKind kind)
	{
		uint32_t start = _selection.Start();
		uint32_t end = _selection.End();
		std::string_view insertion = text;

		if (MaxGraphemes >= 0)
		{
			auto current = static_cast<int64_t>(Unicode::CountGraphemes(_text));
			auto removed = static_cast<int64_t>(Unicode::CountGraphemes(
				std::string_view(_text).substr(start, end - start)
			));
			int64_t available = static_cast<int64_t>(MaxGraphemes) -
								(current - removed);

			if (available <= 0)
			{
				insertion = {};
			}
			else if (
				auto insertedGraphemes =
					static_cast<int64_t>(Unicode::CountGraphemes(insertion));
				insertedGraphemes > available
			)
			{
				uint32_t cursor = 0;
				int64_t count = 0;
				while (cursor < insertion.size() && count < available)
				{
					cursor = Unicode::NextGraphemeBoundary(insertion, cursor);
					count++;
				}
				insertion = insertion.substr(0, cursor);
			}
		}

		if (start == end && insertion.empty())
		{
			return;
		}

		_pushUndo(kind);

		std::string result;
		result.reserve(_text.size() - (end - start) + insertion.size());
		result.append(_text, 0, start);
		result.append(insertion);
		result.append(_text, end);
		_text = std::move(result);

		auto caret = start + static_cast<uint32_t>(insertion.size());
		_selection = TextSelection{
			.Anchor = caret,
			.Active = caret,
			.ActiveAffinity = CaretAffinity::Downstream,
		};
		_preferredX.reset();
	}

	void TextEditor::InsertText(std::string_view utf8)
	{
		CancelPreedit();
		if (ReadOnly)
		{
			return;
		}
		_insertSanitized(_sanitize(utf8), EditKind::Insert);
	}

	void TextEditor::ReplaceSelection(std::string_view utf8)
	{
		InsertText(utf8);
	}

	void TextEditor::_deleteRange(uint32_t start, uint32_t end, EditKind kind)
	{
		auto size = static_cast<uint32_t>(_text.size());
		start = std::min(start, size);
		end = std::min(end, size);
		if (start > end)
		{
			std::swap(start, end);
		}
		if (start == end)
		{
			return;
		}

		_pushUndo(kind);
		_text.erase(start, end - start);

		uint32_t caret = std::min(start, static_cast<uint32_t>(_text.size()));
		_selection = TextSelection{
			.Anchor = caret,
			.Active = caret,
			.ActiveAffinity = CaretAffinity::Downstream,
		};
		_preferredX.reset();
	}

	void TextEditor::DeleteBackward(bool word)
	{
		CancelPreedit();
		if (ReadOnly)
		{
			return;
		}

		if (!_selection.IsEmpty())
		{
			_deleteRange(
				_selection.Start(),
				_selection.End(),
				EditKind::DeleteBackward
			);
			return;
		}

		uint32_t caret = _selection.Active;
		uint32_t start = word ? _previousWordStart(caret)
							  : Unicode::PrevBackspaceBoundary(_text, caret);
		_deleteRange(start, caret, EditKind::DeleteBackward);
	}

	void TextEditor::DeleteForward(bool word)
	{
		CancelPreedit();
		if (ReadOnly)
		{
			return;
		}

		if (!_selection.IsEmpty())
		{
			_deleteRange(
				_selection.Start(),
				_selection.End(),
				EditKind::DeleteForward
			);
			return;
		}

		uint32_t caret = _selection.Active;
		uint32_t end = Unicode::NextGraphemeBoundary(_text, caret);

		if (word)
		{
			end = _nextWordEnd(caret);
			auto size = static_cast<uint32_t>(_text.size());
			while (end < size)
			{
				uint32_t next = Unicode::NextWordBoundary(_text, end);
				uint32_t segmentBegin = end;
				uint32_t segmentEnd = next;
				if (next <= end ||
					segmentIsWord(_text, segmentBegin, segmentEnd))
				{
					break;
				}
				end = next;
			}
		}

		_deleteRange(caret, end, EditKind::DeleteForward);
	}

	auto TextEditor::_previousWordStart(uint32_t offset) const -> uint32_t
	{
		uint32_t cursor = std::min(offset, static_cast<uint32_t>(_text.size()));
		while (cursor > 0)
		{
			uint32_t previous = Unicode::PrevWordBoundary(_text, cursor);
			if (previous >= cursor)
			{
				break;
			}
			if (segmentIsWord(_text, previous, cursor))
			{
				return previous;
			}
			cursor = previous;
		}
		return 0;
	}

	auto TextEditor::_nextWordStart(uint32_t offset) const -> uint32_t
	{
		auto size = static_cast<uint32_t>(_text.size());
		uint32_t cursor = std::min(offset, size);

		while (cursor < size)
		{
			uint32_t next = Unicode::NextWordBoundary(_text, cursor);
			if (next <= cursor)
			{
				break;
			}

			if (segmentIsWord(_text, cursor, next))
			{
				cursor = next;
				while (cursor < size)
				{
					uint32_t following =
						Unicode::NextWordBoundary(_text, cursor);
					if (following <= cursor)
					{
						return size;
					}
					if (segmentIsWord(_text, cursor, following))
					{
						return cursor;
					}
					cursor = following;
				}
				return size;
			}

			cursor = next;
		}

		return size;
	}

	auto TextEditor::_nextWordEnd(uint32_t offset) const -> uint32_t
	{
		auto size = static_cast<uint32_t>(_text.size());
		uint32_t cursor = std::min(offset, size);

		while (cursor < size)
		{
			uint32_t next = Unicode::NextWordBoundary(_text, cursor);
			if (next <= cursor)
			{
				break;
			}
			if (segmentIsWord(_text, cursor, next))
			{
				return next;
			}
			cursor = next;
		}

		return size;
	}

	auto TextEditor::_wordRangeAt(uint32_t offset) const
		-> std::pair<uint32_t, uint32_t>
	{
		if (_text.empty())
		{
			return {0, 0};
		}

		auto size = static_cast<uint32_t>(_text.size());
		uint32_t cursor = _snapOffset(offset);

		uint32_t start = Unicode::PrevWordBoundary(_text, cursor);
		uint32_t end = Unicode::NextWordBoundary(_text, start);

		if (cursor >= end && cursor < size)
		{
			start = cursor;
			end = Unicode::NextWordBoundary(_text, cursor);
		}
		else if (cursor == size)
		{
			end = size;
		}

		return {_snapOffset(start), _snapOffset(end)};
	}

	auto TextEditor::_lineRangeAt(
		uint32_t offset,
		const TextLayout& layout
	) const -> std::pair<uint32_t, uint32_t>
	{
		if (layout.Lines.empty())
		{
			return {0, 0};
		}

		TextPosition position{
			.Offset = _snapOffset(offset),
			.Affinity = CaretAffinity::Downstream,
		};
		uint32_t line = TextGeometry::LineOf(layout, position);
		return {layout.Lines[line].Start, layout.Lines[line].End};
	}

	void TextEditor::_moveVertical(
		int64_t delta,
		bool extend,
		const TextLayout& layout,
		float boxWidth,
		styles::TextAlignment align
	)
	{
		if (layout.Lines.empty() || layout.LineHeight <= 0.0F)
		{
			BreakUndoCoalescing();
			return;
		}

		TextPosition caret = Caret();
		uint32_t line = TextGeometry::LineOf(layout, caret);
		if (line >= layout.Lines.size())
		{
			line = static_cast<uint32_t>(layout.Lines.size() - 1);
		}

		if (!_preferredX)
		{
			_preferredX =
				TextGeometry::CaretAt(layout, caret, boxWidth, align).X;
		}

		int64_t target = static_cast<int64_t>(line) + delta;
		target = std::clamp<int64_t>(
			target,
			0,
			static_cast<int64_t>(layout.Lines.size() - 1)
		);
		int64_t currentLine = line;
		if (target == currentLine)
		{
			BreakUndoCoalescing();
			return;
		}

		float y = (static_cast<float>(target) + 0.5F) * layout.LineHeight;
		TextPosition position = TextGeometry::PositionAtPoint(
			layout,
			Vector2{.X = *_preferredX, .Y = y},
			boxWidth,
			align
		);
		_applyCaret(position, extend, true);
		BreakUndoCoalescing();
	}

	void TextEditor::Move(
		CaretMove kind,
		bool extend,
		const TextLayout& layout,
		float boxWidth,
		styles::TextAlignment align
	)
	{
		CancelPreedit();

		if (!extend && !_selection.IsEmpty())
		{
			bool useStart = kind == CaretMove::GraphemeLeft ||
							kind == CaretMove::WordLeft ||
							kind == CaretMove::LineUp ||
							kind == CaretMove::PageUp ||
							kind == CaretMove::LineStart ||
							kind == CaretMove::DocStart;
			uint32_t collapse = useStart ? _selection.Start()
										 : _selection.End();
			_selection = TextSelection{
				.Anchor = collapse,
				.Active = collapse,
				.ActiveAffinity = CaretAffinity::Downstream,
			};
		}

		TextPosition caret = Caret();
		bool softWrap = isSoftWrapBoundary(layout, caret.Offset);

		if (softWrap && caret.Affinity == CaretAffinity::Upstream &&
			(kind == CaretMove::GraphemeRight || kind == CaretMove::WordRight))
		{
			_applyCaret(
				TextPosition{
					.Offset = caret.Offset,
					.Affinity = CaretAffinity::Downstream,
				},
				extend,
				false
			);
			BreakUndoCoalescing();
			return;
		}

		if (softWrap && caret.Affinity == CaretAffinity::Downstream &&
			(kind == CaretMove::GraphemeLeft || kind == CaretMove::WordLeft))
		{
			_applyCaret(
				TextPosition{
					.Offset = caret.Offset,
					.Affinity = CaretAffinity::Upstream,
				},
				extend,
				false
			);
			BreakUndoCoalescing();
			return;
		}

		switch (kind)
		{
		case CaretMove::GraphemeLeft:
		{
			uint32_t previous =
				Unicode::PrevGraphemeBoundary(_text, caret.Offset);
			auto affinity = isSoftWrapBoundary(layout, previous)
								? CaretAffinity::Upstream
								: CaretAffinity::Downstream;
			_applyCaret(
				TextPosition{.Offset = previous, .Affinity = affinity},
				extend,
				false
			);
			break;
		}
		case CaretMove::GraphemeRight:
		{
			uint32_t next = Unicode::NextGraphemeBoundary(_text, caret.Offset);
			_applyCaret(
				TextPosition{
					.Offset = next,
					.Affinity = CaretAffinity::Downstream,
				},
				extend,
				false
			);
			break;
		}
		case CaretMove::WordLeft:
		{
			uint32_t previous = _previousWordStart(caret.Offset);
			auto affinity = isSoftWrapBoundary(layout, previous)
								? CaretAffinity::Upstream
								: CaretAffinity::Downstream;
			_applyCaret(
				TextPosition{.Offset = previous, .Affinity = affinity},
				extend,
				false
			);
			break;
		}
		case CaretMove::WordRight:
		{
			uint32_t next = WordRightStopsAtWordEnd
								? _nextWordEnd(caret.Offset)
								: _nextWordStart(caret.Offset);
			_applyCaret(
				TextPosition{
					.Offset = next,
					.Affinity = CaretAffinity::Downstream,
				},
				extend,
				false
			);
			break;
		}
		case CaretMove::LineStart:
		{
			TextPosition position = TextGeometry::LineStart(layout, caret);
			_applyCaret(position, extend, false);
			break;
		}
		case CaretMove::LineEnd:
		{
			TextPosition position = TextGeometry::LineEnd(layout, caret);
			_applyCaret(position, extend, false);
			break;
		}
		case CaretMove::DocStart:
			_applyCaret(
				TextPosition{
					.Offset = 0,
					.Affinity = CaretAffinity::Downstream,
				},
				extend,
				false
			);
			break;
		case CaretMove::DocEnd:
			_applyCaret(
				TextPosition{
					.Offset = static_cast<uint32_t>(_text.size()),
					.Affinity = CaretAffinity::Upstream,
				},
				extend,
				false
			);
			break;
		case CaretMove::LineUp:
			_moveVertical(-1, extend, layout, boxWidth, align);
			return;
		case CaretMove::LineDown:
			_moveVertical(1, extend, layout, boxWidth, align);
			return;
		case CaretMove::PageUp:
			_moveVertical(
				-static_cast<int64_t>(PageLines),
				extend,
				layout,
				boxWidth,
				align
			);
			return;
		case CaretMove::PageDown:
			_moveVertical(
				static_cast<int64_t>(PageLines),
				extend,
				layout,
				boxWidth,
				align
			);
			return;
		}

		BreakUndoCoalescing();
	}

	void TextEditor::PointerDown(
		Vector2 localPoint,
		const Modifiers& mods,
		int clickCount,
		const TextLayout& layout,
		float boxWidth,
		styles::TextAlignment align
	)
	{
		CancelPreedit();
		_preferredX.reset();

		TextPosition position =
			TextGeometry::PositionAtPoint(layout, localPoint, boxWidth, align);

		_drag.Active = true;
		if (clickCount >= 3)
		{
			_drag.Granularity = 3;
		}
		else if (clickCount == 2)
		{
			_drag.Granularity = 2;
		}
		else
		{
			_drag.Granularity = 1;
		}

		if (_drag.Granularity == 1)
		{
			_applyCaret(position, mods.Shift, false);
			_drag.Anchor = _selection.Anchor;
			BreakUndoCoalescing();
			return;
		}

		auto range = _drag.Granularity == 2
						 ? _wordRangeAt(position.Offset)
						 : _lineRangeAt(position.Offset, layout);

		if (mods.Shift && !_selection.IsEmpty())
		{
			_selection.Active = position.Offset < _selection.Anchor
									? range.first
									: range.second;
			_selection.ActiveAffinity = CaretAffinity::Downstream;
			_drag.Anchor = _selection.Anchor;
		}
		else
		{
			_selection = TextSelection{
				.Anchor = range.first,
				.Active = range.second,
				.ActiveAffinity = CaretAffinity::Downstream,
			};
			_drag.Anchor = range.first;
		}

		BreakUndoCoalescing();
	}

	void TextEditor::PointerDrag(
		Vector2 localPoint,
		const TextLayout& layout,
		float boxWidth,
		styles::TextAlignment align
	)
	{
		if (!_drag.Active)
		{
			return;
		}

		TextPosition position =
			TextGeometry::PositionAtPoint(layout, localPoint, boxWidth, align);

		if (_drag.Granularity == 1)
		{
			_selection.Active = _snapOffset(position.Offset);
			_selection.ActiveAffinity = position.Affinity;
		}
		else
		{
			auto range = _drag.Granularity == 2
							 ? _wordRangeAt(position.Offset)
							 : _lineRangeAt(position.Offset, layout);
			_selection.Active = position.Offset < _drag.Anchor ? range.first
															   : range.second;
			_selection.ActiveAffinity = CaretAffinity::Downstream;
		}

		_preferredX.reset();
	}

	void TextEditor::PointerUp()
	{
		_drag.Active = false;
		_drag.Granularity = 1;
		_drag.Anchor = 0;
	}

	auto TextEditor::_clampPreeditCaret(uint32_t value) const -> uint32_t
	{
		auto size = static_cast<uint32_t>(_preedit.size());
		uint32_t clamped = std::min(value, size);
		if (clamped == 0 || clamped == size)
		{
			return clamped;
		}

		uint32_t start = Unicode::PrevGraphemeBoundary(_preedit, clamped);
		return Unicode::NextGraphemeBoundary(_preedit, start) == clamped
				   ? clamped
				   : start;
	}

	void TextEditor::SetPreedit(
		std::string_view text,
		uint32_t caretBegin,
		uint32_t caretEnd
	)
	{
		_preedit = _sanitize(text);
		_preeditCaretBegin = _clampPreeditCaret(caretBegin);
		_preeditCaretEnd = _clampPreeditCaret(caretEnd);
		_composing = !_preedit.empty();
	}

	void TextEditor::CommitText(std::string_view committed)
	{
		CancelPreedit();
		InsertText(committed);
	}

	void TextEditor::CancelPreedit()
	{
		_preedit.clear();
		_preeditCaretBegin = 0;
		_preeditCaretEnd = 0;
		_composing = false;
	}

	auto TextEditor::HasComposition() const -> bool
	{
		return _composing && !_preedit.empty();
	}

	auto TextEditor::DisplayText() const -> std::string
	{
		if (!HasComposition())
		{
			return _text;
		}

		uint32_t start = _selection.Start();
		uint32_t end = _selection.End();

		std::string result;
		result.reserve(_text.size() - (end - start) + _preedit.size());
		result.append(_text, 0, start);
		result.append(_preedit);
		result.append(_text, end);
		return result;
	}

	auto TextEditor::CaretInDisplay() const -> TextPosition
	{
		if (!HasComposition())
		{
			return Caret();
		}

		return TextPosition{
			.Offset = _selection.Start() + _clampPreeditCaret(_preeditCaretEnd),
			.Affinity = CaretAffinity::Downstream,
		};
	}

	auto TextEditor::SelectionInDisplay() const -> std::pair<uint32_t, uint32_t>
	{
		if (HasComposition())
		{
			uint32_t start = _selection.Start();
			return {start, start};
		}
		return {_selection.Start(), _selection.End()};
	}

	auto TextEditor::CompositionRange() const -> std::pair<uint32_t, uint32_t>
	{
		if (!HasComposition())
		{
			return {0, 0};
		}

		uint32_t start = _selection.Start();
		return {start, start + static_cast<uint32_t>(_preedit.size())};
	}

	void TextEditor::Undo()
	{
		if (_undo.empty())
		{
			return;
		}

		_redo.push_back(_makeSnapshot());

		Snapshot snapshot = std::move(_undo.back());
		_undo.pop_back();
		_text = std::move(snapshot.Text);
		_selection = snapshot.Selection;
		_clampSelection();

		CancelPreedit();
		BreakUndoCoalescing();
		_preferredX.reset();
	}

	void TextEditor::Redo()
	{
		if (_redo.empty())
		{
			return;
		}

		_undo.push_back(_makeSnapshot());
		if (_undo.size() > MaxUndoEntries)
		{
			_undo.erase(_undo.begin());
		}

		Snapshot snapshot = std::move(_redo.back());
		_redo.pop_back();
		_text = std::move(snapshot.Text);
		_selection = snapshot.Selection;
		_clampSelection();

		CancelPreedit();
		BreakUndoCoalescing();
		_preferredX.reset();
	}

	auto TextEditor::CanUndo() const -> bool
	{
		return !_undo.empty();
	}

	auto TextEditor::CanRedo() const -> bool
	{
		return !_redo.empty();
	}
}

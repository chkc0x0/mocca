#pragma once
#include "input/InputTypes.h"
#include "style/StyleEnums.h"
#include "text/TextGeometry.h"
#include "text/TextLayout.h"
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mocca::mtext
{
	struct TextSelection
	{
		uint32_t Anchor = 0;
		uint32_t Active = 0;
		CaretAffinity ActiveAffinity = CaretAffinity::Downstream;

		[[nodiscard]] auto Start() const -> uint32_t
		{
			return Anchor < Active ? Anchor : Active;
		}

		[[nodiscard]] auto End() const -> uint32_t
		{
			return Anchor < Active ? Active : Anchor;
		}

		[[nodiscard]] auto IsEmpty() const -> bool
		{
			return Anchor == Active;
		}
	};

	enum class CaretMove : uint8_t
	{
		GraphemeLeft,
		GraphemeRight,
		WordLeft,
		WordRight,
		LineUp,
		LineDown,
		LineStart,
		LineEnd,
		DocStart,
		DocEnd,
		PageUp,
		PageDown,
	};

	class TextEditor
	{
	public:
		void SetText(const std::string& text, bool resetUndo = false);
		[[nodiscard]] auto Text() const -> const std::string&;
		[[nodiscard]] auto Selection() const -> const TextSelection&;
		[[nodiscard]] auto Caret() const -> TextPosition;

		void SetCaret(TextPosition position, bool extend = false);
		void SetSelection(TextSelection selection);
		void SelectAll();
		[[nodiscard]] auto SelectedText() const -> std::string;

		void InsertText(std::string_view utf8);
		void ReplaceSelection(std::string_view utf8);
		void DeleteBackward(bool word = false);
		void DeleteForward(bool word = false);

		void Move(
			CaretMove kind,
			bool extend,
			const TextLayout& layout,
			float boxWidth,
			styles::TextAlignment align
		);

		void PointerDown(
			Vector2 localPoint,
			const Modifiers& mods,
			int clickCount,
			const TextLayout& layout,
			float boxWidth,
			styles::TextAlignment align
		);
		void PointerDrag(
			Vector2 localPoint,
			const TextLayout& layout,
			float boxWidth,
			styles::TextAlignment align
		);
		void PointerUp();

		void SetPreedit(
			std::string_view text,
			uint32_t caretBegin,
			uint32_t caretEnd
		);
		void CommitText(std::string_view committed);
		void CancelPreedit();
		[[nodiscard]] auto HasComposition() const -> bool;

		[[nodiscard]] auto DisplayText() const -> std::string;
		[[nodiscard]] auto CaretInDisplay() const -> TextPosition;
		[[nodiscard]] auto SelectionInDisplay() const
			-> std::pair<uint32_t, uint32_t>;
		[[nodiscard]] auto CompositionRange() const
			-> std::pair<uint32_t, uint32_t>;

		void Undo();
		void Redo();
		[[nodiscard]] auto CanUndo() const -> bool;
		[[nodiscard]] auto CanRedo() const -> bool;
		void BreakUndoCoalescing();

		bool Multiline = false;
		bool ReadOnly = false;
		bool WordRightStopsAtWordEnd = true;
		int MaxGraphemes = -1;
		uint32_t PageLines = 10;

	private:
		enum class EditKind : uint8_t
		{
			None,
			Insert,
			DeleteBackward,
			DeleteForward,
		};

		struct Snapshot
		{
			std::string Text;
			TextSelection Selection;
		};

		struct DragState
		{
			bool Active = false;
			uint8_t Granularity = 1;
			uint32_t Anchor = 0;
		};

		std::string _text;
		TextSelection _selection;
		EditKind _lastEditKind = EditKind::None;
		bool _coalescing = false;
		bool _composing = false;
		DragState _drag;
		std::vector<Snapshot> _undo;
		std::vector<Snapshot> _redo;
		std::optional<float> _preferredX;
		std::string _preedit;
		uint32_t _preeditCaretBegin = 0;
		uint32_t _preeditCaretEnd = 0;

		[[nodiscard]] auto _sanitize(std::string_view text) const
			-> std::string;
		[[nodiscard]] auto _snapOffset(uint32_t offset) const -> uint32_t;
		void _clampSelection();

		[[nodiscard]] auto _makeSnapshot() const -> Snapshot;
		void _pushUndo(EditKind kind);
		void
		_applyCaret(TextPosition position, bool extend, bool keepPreferred);

		void _insertSanitized(std::string_view text, EditKind kind);
		void _deleteRange(uint32_t start, uint32_t end, EditKind kind);

		[[nodiscard]] auto _previousWordStart(uint32_t offset) const
			-> uint32_t;
		[[nodiscard]] auto _nextWordStart(uint32_t offset) const -> uint32_t;
		[[nodiscard]] auto _nextWordEnd(uint32_t offset) const -> uint32_t;
		[[nodiscard]] auto _wordRangeAt(uint32_t offset) const
			-> std::pair<uint32_t, uint32_t>;
		[[nodiscard]] auto _lineRangeAt(
			uint32_t offset,
			const TextLayout& layout
		) const -> std::pair<uint32_t, uint32_t>;

		void _moveVertical(
			int64_t delta,
			bool extend,
			const TextLayout& layout,
			float boxWidth,
			styles::TextAlignment align
		);
		[[nodiscard]] auto _clampPreeditCaret(uint32_t value) const -> uint32_t;
	};
}

#pragma once
#include "input/InputTypes.h"
#include "text/TextEditor.h"
#include <cstdint>

namespace mocca::mtext
{
	enum class EditCommand : uint8_t
	{
		None,
		Move,
		DeleteBackward,
		DeleteWordBackward,
		DeleteForward,
		DeleteWordForward,
		SelectAll,
		Copy,
		Cut,
		Paste,
		Undo,
		Redo,
		InsertNewline,
		InsertTab,
		Submit,
	};

	struct EditAction
	{
		EditCommand Command = EditCommand::None;
		CaretMove Move{};
		bool Extend = false;

		[[nodiscard]] static auto ResolveEditKey(const KeyEvent& event, bool multiline)
			-> EditAction;
	};
}

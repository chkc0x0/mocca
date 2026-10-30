#include "text/TextKeymap.h"
#include <optional>

namespace mocca::mtext
{
	auto moveAction(CaretMove kind, bool extend) -> EditAction
	{
		return EditAction{
			.Command = EditCommand::Move,
			.Move = kind,
			.Extend = extend,
		};
	}

	auto resolveNavigationKey(
		const KeyEvent& event,
		bool command,
		bool extend,
		bool multiline
	) -> std::optional<EditAction>
	{
		switch (event.Code)
		{
		case KeyCode::Left:
			return moveAction(
				command ? CaretMove::WordLeft : CaretMove::GraphemeLeft,
				extend
			);
		case KeyCode::Right:
			return moveAction(
				command ? CaretMove::WordRight : CaretMove::GraphemeRight,
				extend
			);
		case KeyCode::Up:
			return moveAction(CaretMove::LineUp, extend);
		case KeyCode::Down:
			return moveAction(CaretMove::LineDown, extend);
		case KeyCode::Home:
			return moveAction(
				command && multiline ? CaretMove::DocStart
									 : CaretMove::LineStart,
				extend
			);
		case KeyCode::End:
			return moveAction(
				command && multiline ? CaretMove::DocEnd : CaretMove::LineEnd,
				extend
			);
		case KeyCode::PageUp:
			return moveAction(CaretMove::PageUp, extend);
		case KeyCode::PageDown:
			return moveAction(CaretMove::PageDown, extend);
		default:
			return std::nullopt;
		}
	}

	auto resolveClipboardKey(const KeyEvent& event, bool command, bool shift)
		-> std::optional<EditAction>
	{
		switch (event.Code)
		{
		case KeyCode::Insert:
			if (command)
			{
				return EditAction{.Command = EditCommand::Copy};
			}
			if (shift)
			{
				return EditAction{.Command = EditCommand::Paste};
			}
			return EditAction{};
		case KeyCode::A:
			return command ? std::optional<EditAction>(
								 EditAction{.Command = EditCommand::SelectAll}
							 )
						   : std::nullopt;
		case KeyCode::C:
			return command ? std::optional<EditAction>(
								 EditAction{.Command = EditCommand::Copy}
							 )
						   : std::nullopt;
		case KeyCode::X:
			return command ? std::optional<EditAction>(
								 EditAction{.Command = EditCommand::Cut}
							 )
						   : std::nullopt;
		case KeyCode::V:
			return command ? std::optional<EditAction>(
								 EditAction{.Command = EditCommand::Paste}
							 )
						   : std::nullopt;
		default:
			return std::nullopt;
		}
	}

	auto resolveEditCommandKey(
		const KeyEvent& event,
		bool command,
		bool shift,
		bool multiline
	) -> std::optional<EditAction>
	{
		switch (event.Code)
		{
		case KeyCode::Backspace:
			return EditAction{
				.Command = command ? EditCommand::DeleteWordBackward
								   : EditCommand::DeleteBackward,
			};
		case KeyCode::Delete:
			if (command)
			{
				return EditAction{
					.Command = EditCommand::DeleteWordForward,
				};
			}
			if (shift)
			{
				return EditAction{.Command = EditCommand::Cut};
			}
			return EditAction{.Command = EditCommand::DeleteForward};
		case KeyCode::Z:
			if (!command)
			{
				return std::nullopt;
			}
			return EditAction{
				.Command = shift ? EditCommand::Redo : EditCommand::Undo,
			};
		case KeyCode::Y:
			return command ? std::optional<EditAction>(
								 EditAction{.Command = EditCommand::Redo}
							 )
						   : std::nullopt;
		case KeyCode::Enter:
			return EditAction{
				.Command = multiline ? EditCommand::InsertNewline
									 : EditCommand::Submit,
			};
		default:
			return std::nullopt;
		}
	}
	
	auto EditAction::ResolveEditKey(const KeyEvent& event, bool multiline)
		-> EditAction
	{
		if (event.EventType == KeyEvent::Type::Up)
		{
			return EditAction{};
		}

		bool command = event.Mods.Ctrl;
		bool shift = event.Mods.Shift;

		if (auto action =
				resolveNavigationKey(event, command, shift, multiline))
		{
			return *action;
		}
		if (auto action = resolveClipboardKey(event, command, shift))
		{
			return *action;
		}
		if (auto action =
				resolveEditCommandKey(event, command, shift, multiline))
		{
			return *action;
		}
		return EditAction{};
	}
}

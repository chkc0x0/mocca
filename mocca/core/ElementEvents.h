#pragma once
#include "input/InputTypes.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <variant>
#include <algorithm>

namespace mocca
{
	using PointerCallback = std::function<void(PointerEvent&)>;
	using KeyCallback = std::function<void(KeyEvent&)>;
	using TextCallback = std::function<void(TextEvent&)>;
	using FocusCallback = std::function<void(FocusEvent&)>;

	enum class EventType : uint8_t
	{
		PointerEnter,
		PointerLeave,
		PointerDown,
		PointerUp,
		PointerMove,
		KeyDown,
		KeyUp,
		KeyRepeat,
		TextInput,
		Focus,
		Blur,
	};

	struct EventHandlers
	{
		PointerCallback OnPointerEnter;
		PointerCallback OnPointerLeave;
		PointerCallback OnPointerDown;
		PointerCallback OnPointerUp;
		PointerCallback OnPointerMove;
		KeyCallback OnKeyDown;
		KeyCallback OnKeyUp;
		KeyCallback OnKeyRepeat;
		TextCallback OnTextInput;
		FocusCallback OnFocus;
		FocusCallback OnBlur;
	};

	struct EventView
	{
		using HandlerFn = std::variant<
			PointerCallback,
			KeyCallback,
			TextCallback,
			FocusCallback>;

		[[nodiscard]] auto Has(EventType t) const -> bool
		{
			return (_mask & (1U << static_cast<uint8_t>(t))) != 0U;
		}

		void Set(EventType t, HandlerFn fn)
		{
			auto it = std::lower_bound(
				_entries.begin(),
				_entries.end(),
				t,
				[](const Entry& e, EventType t) -> bool { return e.Type < t; }
			);
			if (it != _entries.end() && it->Type == t)
			{
				it->Callback = std::move(fn);
			}
			else
			{
				_entries.insert(it, Entry{.Type=t, .Callback=std::move(fn)});
			}
			_mask |= 1U << static_cast<uint8_t>(t);
		}

		template <class E> void Call(EventType t, E& ev) const
		{
			if (!Has(t))
			{
				return;
			}
			auto it = std::lower_bound(
				_entries.begin(),
				_entries.end(),
				t,
				[](const Entry& e, EventType t) -> bool { return e.Type < t; }
			);
			std::get<std::function<void(E&)>>(it->Callback)(ev);
		}

		static auto From(EventHandlers&& h) -> std::shared_ptr<EventView>
		{
			std::shared_ptr<EventView> out;
			auto add = [&](EventType t, auto& fn)
		 -> auto	{
				if (!fn)
				{
					return;
				}
				if (!out)
				{
					out = std::make_shared<EventView>();
				}
				out->Set(t, std::move(fn));
			};
			
			add(EventType::PointerEnter, h.OnPointerEnter);
			add(EventType::PointerLeave, h.OnPointerLeave);
			add(EventType::PointerDown, h.OnPointerDown);
			add(EventType::PointerUp, h.OnPointerUp);
			add(EventType::PointerMove, h.OnPointerMove);
			add(EventType::KeyDown, h.OnKeyDown);
			add(EventType::KeyUp, h.OnKeyUp);
			add(EventType::KeyRepeat, h.OnKeyRepeat);
			add(EventType::TextInput, h.OnTextInput);
			add(EventType::Focus, h.OnFocus);
			add(EventType::Blur, h.OnBlur);
			return out;
		}

	private:
		struct Entry
		{
			EventType Type;
			HandlerFn Callback;
		};
		uint16_t _mask = 0;
		std::vector<Entry> _entries;
	};
}
#pragma once
#include "input/InputTypes.h"
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <vector>

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
		Activate,
		A11yAction,
	};

	constexpr uint32_t FirstCustomEventId = 1U << 16;

	namespace detail
	{
		inline auto nextCustomEventId() -> uint32_t
		{
			static std::atomic<uint32_t> next{FirstCustomEventId};
			return next.fetch_add(1, std::memory_order_relaxed);
		}
	}

	template <class E> struct EventKey
	{
		using Payload = E;
		uint32_t Id;
	};

	namespace events
	{
		inline constexpr EventKey<PointerEvent> PointerEnter{
			static_cast<uint32_t>(EventType::PointerEnter),
		};
		inline constexpr EventKey<PointerEvent> PointerLeave{
			static_cast<uint32_t>(EventType::PointerLeave),
		};
		inline constexpr EventKey<PointerEvent> PointerDown{
			static_cast<uint32_t>(EventType::PointerDown),
		};
		inline constexpr EventKey<PointerEvent> PointerUp{
			static_cast<uint32_t>(EventType::PointerUp),
		};
		inline constexpr EventKey<PointerEvent> PointerMove{
			static_cast<uint32_t>(EventType::PointerMove),
		};
		inline constexpr EventKey<KeyEvent> KeyDown{
			static_cast<uint32_t>(EventType::KeyDown),
		};
		inline constexpr EventKey<KeyEvent> KeyUp{
			static_cast<uint32_t>(EventType::KeyUp),
		};
		inline constexpr EventKey<KeyEvent> KeyRepeat{
			static_cast<uint32_t>(EventType::KeyRepeat),
		};
		inline constexpr EventKey<TextEvent> TextInput{
			static_cast<uint32_t>(EventType::TextInput),
		};
		inline constexpr EventKey<FocusEvent> Focus{
			static_cast<uint32_t>(EventType::Focus),
		};
		inline constexpr EventKey<FocusEvent> Blur{
			static_cast<uint32_t>(EventType::Blur),
		};
		inline constexpr EventKey<ActivateEvent> Activate{
			static_cast<uint32_t>(EventType::Activate),
		};
		inline constexpr EventKey<A11yActionEvent> A11yAction{
			static_cast<uint32_t>(EventType::A11yAction),
		};
	}

	template <class E> struct CustomEvent
	{
		using Payload = E;

		static auto Key() -> EventKey<E>
		{
			static const uint32_t Id = detail::nextCustomEventId();
			return EventKey<E>{Id};
		}
	};

	struct EventEntry
	{
		uint32_t Id = 0;
		const std::type_info* Payload = nullptr;
		std::shared_ptr<void> Handler;
		void (*Invoke)(void*, void*) = nullptr;
	};

	template <class E, class F> auto on(EventKey<E> key, F&& fn) -> EventEntry
	{
		static_assert(
			std::is_invocable_v<F&, E&>,
			"event handler must be callable with the event payload"
		);

		using Fn = std::function<void(E&)>;
		auto handler = std::make_shared<Fn>(std::forward<F>(fn));

		return EventEntry{
			.Id = key.Id,
			.Payload = &typeid(E),
			.Handler = handler,
			.Invoke = [](void* h, void* ev) -> void
			{ (*static_cast<Fn*>(h))(*static_cast<E*>(ev)); },
		};
	}

	template <class E, class F> auto on(CustomEvent<E> ev, F&& fn) -> EventEntry
	{
		return on(CustomEvent<E>::Key(), std::forward<F>(fn));
	}

	struct EventView
	{
		EventView() = default;

		template <class... Entries>
			requires(
				std::same_as<std::remove_cvref_t<Entries>, EventEntry> && ...
			)
		EventView(Entries&&... entries)
		{
			(Add(std::forward<Entries>(entries)), ...);
		}

		[[nodiscard]] auto Has(EventType t) const -> bool
		{
			auto bit = static_cast<uint32_t>(t);
			return bit < 64 && (_mask & (1ULL << bit)) != 0;
		}

		[[nodiscard]] auto Empty() const -> bool
		{
			return _entries.empty();
		}

		void Add(EventEntry entry)
		{
			auto id = entry.Id;
			auto it = std::upper_bound(
				_entries.begin(),
				_entries.end(),
				id,
				[](uint32_t id, const EventEntry& e) -> bool
				{ return id < e.Id; }
			);
			_entries.insert(it, std::move(entry));
			if (id < 64)
			{
				_mask |= 1ULL << id;
			}
		}

		template <class E> void Call(EventKey<E> key, E& ev) const
		{
			_callRaw(key.Id, typeid(E), &ev);
		}

		template <class E> void Call(EventType type, E& ev) const
		{
			_callRaw(static_cast<uint32_t>(type), typeid(E), &ev);
		}

		template <class E> void Call(CustomEvent<E> ev, E& cb) const
		{
			Call(CustomEvent<E>::Key(), cb);
		}

		template <class E> void Emit(CustomEvent<E> ev, E& cb) const
		{
			Call(CustomEvent<E>::Key(), cb);
		}

		static auto From(EventView&& view) -> std::shared_ptr<EventView>
		{
			if (view.Empty())
			{
				return nullptr;
			}
			return std::make_shared<EventView>(std::move(view));
		}

	private:
		friend class NodeRef;

		void _callRaw(uint32_t id, const std::type_info& type, void* ev) const
		{
			if (id < 64 && (_mask & (1ULL << id)) == 0)
			{
				return;
			}

			auto it = std::lower_bound(
				_entries.begin(),
				_entries.end(),
				id,
				[](const EventEntry& e, uint32_t id) -> bool
				{ return e.Id < id; }
			);

			for (; it != _entries.end() && it->Id == id; ++it)
			{
				if (it->Payload != nullptr && *it->Payload == type &&
					it->Invoke != nullptr)
				{
					it->Invoke(it->Handler.get(), ev);
				}
			}
		}

		uint64_t _mask = 0;
		std::vector<EventEntry> _entries;
	};
}
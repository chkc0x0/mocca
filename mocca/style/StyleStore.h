#pragma once
#include "ComputedStyle.h"
#include <cstddef>
#include <cstdint>
#include <deque>
#include <vector>

namespace mocca::styles
{
	class StyleStore
	{
	public:
		StyleStore();

		auto Intern(const ComputedStyle& value) -> StyleId;
		auto MakeUnique(const ComputedStyle& value) -> StyleId;
		auto Mut(StyleId id) -> ComputedStyle&;
		auto Settle(StyleId id) -> StyleId;
		void Release(StyleId id);

		[[nodiscard]] auto Resolve(StyleId id) const -> const ComputedStyle&;
		[[nodiscard]] auto IsUnique(StyleId id) const -> bool;

		[[nodiscard]] auto InternedCount() const -> std::size_t
		{
			return _interned;
		}
		[[nodiscard]] auto UniqueCount() const -> std::size_t
		{
			return _unique;
		}

	private:
		enum class SlotKind : std::uint8_t
		{
			Free,
			Interned,
			Unique,
		};

		struct Slot
		{
			SlotKind State = SlotKind::Free;
			std::uint32_t NextFree = 0;
		};

		auto _allocate() -> StyleId;
		auto _find(const ComputedStyle& value, std::size_t hash) const
			-> StyleId;
		void _insert(StyleId id, std::size_t hash);
		void _rehash(std::size_t capacity);

		std::deque<ComputedStyle> _values;
		std::vector<Slot> _slots;
		std::vector<StyleId> _buckets;
		StyleId _freeHead = 0;
		StyleId _lastHit = 0;
		std::size_t _interned = 0;
		std::size_t _unique = 0;
	};
}

namespace mocca::styles::detail
{
	auto styleStore() -> StyleStore&;
}

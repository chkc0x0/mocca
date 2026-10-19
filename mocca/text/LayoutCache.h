#pragma once
#include "text/TextLayout.h"
#include <cstddef>
#include <list>
#include <memory>
#include <unordered_map>

namespace mocca::mtext::detail
{
	struct LayoutKey
	{
		uint64_t ShapedId = 0;
		uint32_t SizeBits = 0;
		uint8_t LineHeightKind = 0;
		uint32_t LineHeightBits = 0;
		uint32_t LetterSpacingBits = 0;
		uint32_t WordSpacingBits = 0;
		uint32_t MaxWidthBits = 0;
		bool Wrap = false;

		auto operator==(const LayoutKey&) const -> bool = default;
	};

	struct LayoutKeyHash
	{
		auto operator()(const LayoutKey& key) const -> size_t;
	};

	class LayoutCache
	{
	public:
		explicit LayoutCache(size_t capacity) noexcept : _capacity(capacity)
		{
		}

		auto Find(const LayoutKey& key) -> std::shared_ptr<const TextLayout>;

		void
		Insert(const LayoutKey& key, std::shared_ptr<const TextLayout> value);

		void Clear();

	private:
		struct Entry
		{
			LayoutKey Key;
			std::shared_ptr<const TextLayout> Value;
		};

		size_t _capacity;
		std::list<Entry> _entries;
		std::unordered_map<LayoutKey, std::list<Entry>::iterator, LayoutKeyHash>
			_index;
	};
}
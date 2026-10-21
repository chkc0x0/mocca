#pragma once
#include "text/FontTypes.h"
#include "text/TextLayout.h"
#include <cstddef>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace mocca::mtext::detail
{
	struct ShapeKey
	{
		std::string Content;
		std::vector<FontFaceId> Faces;
		size_t Variations = 0;
		uint32_t Features = 0;
		BaseDirection Base = BaseDirection::LTR;

		auto operator==(const ShapeKey&) const -> bool = default;
	};

	class ShapeCache
	{
	public:
		explicit ShapeCache(size_t capacity) noexcept : _capacity(capacity)
		{
		}

		auto Find(const ShapeKey& key) -> std::shared_ptr<const ShapedText>;

		void
		Insert(const ShapeKey& key, std::shared_ptr<const ShapedText> value);

		void Clear();

	private:
		struct Entry
		{
			ShapeKey Key;
			std::shared_ptr<const ShapedText> Value;
		};

		void _evict(const ShapeKey& key);

		size_t _capacity;
		std::list<Entry> _entries;
		std::unordered_multimap<size_t, std::list<Entry>::iterator> _index;
	};
}
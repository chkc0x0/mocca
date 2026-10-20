#include "text/ShapeCache.h"
#include "core/Identifiers.h"

namespace mocca::mtext::detail
{
	auto hashShapeKey(const ShapeKey& key) -> size_t
	{
		size_t hash = std::hash<std::string>{}(key.Content);
		for (FontFaceId face : key.Faces)
		{
			mocca::hashCombine(hash, std::hash<size_t>{}(face));
		}
		mocca::hashCombine(hash, key.Variations);
		mocca::hashCombine(hash, std::hash<uint32_t>{}(key.Features));
		return hash;
	}

	auto ShapeCache::Find(const ShapeKey& key)
		-> std::shared_ptr<const ShapedText>
	{
		auto range = _index.equal_range(hashShapeKey(key));
		for (auto it = range.first; it != range.second; ++it)
		{
			if (it->second->Key == key)
			{
				_entries.splice(_entries.begin(), _entries, it->second);
				return it->second->Value;
			}
		}
		return nullptr;
	}

	void ShapeCache::Insert(
		const ShapeKey& key,
		std::shared_ptr<const ShapedText> value
	)
	{
		_entries.push_front(Entry{.Key = key, .Value = std::move(value)});
		_index.emplace(hashShapeKey(key), _entries.begin());

		while (_entries.size() > _capacity)
		{
			_evict(_entries.back().Key);
		}
	}

	void ShapeCache::Clear()
	{
		_entries.clear();
		_index.clear();
	}

	void ShapeCache::_evict(const ShapeKey& key)
	{
		auto range = _index.equal_range(hashShapeKey(key));
		for (auto it = range.first; it != range.second; ++it)
		{
			if (it->second == std::prev(_entries.end()))
			{
				_index.erase(it);
				break;
			}
		}
		_entries.pop_back();
	}
}
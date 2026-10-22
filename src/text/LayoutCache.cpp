#include "text/LayoutCache.h"
#include "core/Identifiers.h"

namespace mocca::mtext::detail
{
	auto LayoutKeyHash::operator()(const LayoutKey& key) const -> size_t
	{
		size_t hash = std::hash<uint64_t>{}(key.ShapedId);
		mocca::detail::hashCombine(hash, std::hash<uint32_t>{}(key.SizeBits));
		mocca::detail::hashCombine(
			hash,
			std::hash<uint8_t>{}(key.LineHeightKind)
		);
		mocca::detail::hashCombine(
			hash,
			std::hash<uint32_t>{}(key.LineHeightBits)
		);
		mocca::detail::hashCombine(
			hash,
			std::hash<uint32_t>{}(key.LetterSpacingBits)
		);
		mocca::detail::hashCombine(
			hash,
			std::hash<uint32_t>{}(key.WordSpacingBits)
		);
		mocca::detail::hashCombine(
			hash,
			std::hash<uint32_t>{}(key.MaxWidthBits)
		);
		mocca::detail::hashCombine(hash, std::hash<bool>{}(key.Wrap));
		return hash;
	}

	auto LayoutCache::Find(const LayoutKey& key)
		-> std::shared_ptr<const TextLayout>
	{
		auto it = _index.find(key);
		if (it == _index.end())
		{
			return nullptr;
		}
		_entries.splice(_entries.begin(), _entries, it->second);
		return it->second->Value;
	}

	void LayoutCache::Insert(
		const LayoutKey& key,
		std::shared_ptr<const TextLayout> value
	)
	{
		if (auto it = _index.find(key); it != _index.end())
		{
			it->second->Value = std::move(value);
			_entries.splice(_entries.begin(), _entries, it->second);
			return;
		}

		_entries.push_front(Entry{.Key = key, .Value = std::move(value)});
		_index.emplace(key, _entries.begin());

		while (_entries.size() > _capacity)
		{
			_index.erase(_entries.back().Key);
			_entries.pop_back();
		}
	}

	void LayoutCache::Clear()
	{
		_entries.clear();
		_index.clear();
	}
}
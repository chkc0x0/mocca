#include "gfx/TextureCache.h"
#include "core/Logger.h"

namespace mocca::gfx
{
	TextureCache::TextureCache(
		TextureRegistry& registry,
		std::span<Surface* const> served
	)
		: _registry(registry), _sub(registry.Subscribe(served))
	{
	}

	TextureCache::~TextureCache()
	{
		mc_assert(_entries.empty(), "TextureCache must be shutdown by dtor");
	}

	void TextureCache::Sync(std::span<const Texture* const> textures)
	{
		_seen.clear();

		for (const auto* tex : textures)
		{
			if (tex == nullptr)
			{
				continue;
			}

			_seen.insert(tex->Id);

			auto entry = _entries.find(tex->Id);
			if (entry != _entries.end() &&
				entry->second.Revision == tex->Revision)
			{
				continue;
			}

			if (tex->Pixels.empty())
			{
				if (_warned.insert(tex->Id).second)
				{
					mc_warning(
						"texture {} (rev {}) has no pixels, cannot upload",
						tex->Id.Id,
						tex->Revision
					);
				}
				continue;
			}

			if (entry == _entries.end())
			{
				auto handle = CreateImpl(*tex);
				if (handle == 0)
				{
					mc_warning(
						"texture {} (rev {}) could not be created",
						tex->Id.Id,
						tex->Revision
					);
					continue;
				}

				_entries.emplace(
					tex->Id,
					Entry{
						.Handle = handle,
						.Desc = tex->Desc,
						.Revision = tex->Revision,
					}
				);
			}
			else if (entry->second.Desc != tex->Desc)
			{
				DestroyImpl(entry->second.Handle);

				auto handle = CreateImpl(*tex);
				if (handle == 0)
				{
					_entries.erase(entry);
					continue;
				}

				entry->second = Entry{
					.Handle = handle,
					.Desc = tex->Desc,
					.Revision = tex->Revision,
				};
			}
			else
			{
				UpdateImpl(entry->second.Handle, *tex);
				entry->second.Revision = tex->Revision;
			}

			_warned.erase(tex->Id);
			_registry.Acknowledge(_sub, tex->Id, tex->Revision);
		}

		for (auto it = _entries.begin(); it != _entries.end();)
		{
			if (_seen.contains(it->first))
			{
				++it;
				continue;
			}

			DestroyImpl(it->second.Handle);
			_registry.AcknowledgeRemoved(_sub, it->first);
			_warned.erase(it->first);
			it = _entries.erase(it);
		}
	}

	auto TextureCache::Handle(TextureId id) const -> uintptr_t
	{
		auto entry = _entries.find(id);
		return entry != _entries.end() ? entry->second.Handle : 0;
	}

	void TextureCache::Shutdown()
	{
		for (auto& [Id, entry] : _entries)
		{
			DestroyImpl(entry.Handle);
			_registry.AcknowledgeRemoved(_sub, Id);
		}

		_entries.clear();
		_seen.clear();
		_warned.clear();
		_registry.Unsubscribe(_sub);
	}
}

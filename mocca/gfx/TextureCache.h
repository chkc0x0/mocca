#pragma once
#include "gfx/Texture.h"
#include "gfx/TextureRegistry.h"
#include <span>
#include <unordered_map>
#include <unordered_set>

namespace mocca::gfx
{
	class TextureCache
	{
	public:
		TextureCache(TextureRegistry& registry, std::span<Surface* const> served);
		virtual ~TextureCache();

		void Sync(std::span<const Texture* const> textures);

		[[nodiscard]] auto Handle(TextureId id) const -> uintptr_t;

	protected:
		void Shutdown();

		virtual auto CreateImpl(const Texture& src) -> uintptr_t = 0;
		virtual void UpdateImpl(uintptr_t handle, const Texture& src) = 0;
		virtual void DestroyImpl(uintptr_t handle) = 0;

	private:
		struct Entry
		{
			uintptr_t Handle;
			TextureDesc Desc;
			uint32_t Revision;
		};

		TextureRegistry& _registry;
		TextureRegistry::SubscriberId _sub;
		std::unordered_map<TextureId, Entry, TextureIdHash> _entries;
		std::unordered_set<TextureId, TextureIdHash> _seen;
		std::unordered_set<TextureId, TextureIdHash> _warned;
	};
}

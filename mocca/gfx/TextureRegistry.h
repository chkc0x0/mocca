#pragma once
#include "gfx/Texture.h"
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace mocca
{
	class Surface;
}

namespace mocca::gfx
{
	class TextureRegistry
	{
	public:
		using SubscriberId = uint32_t;

		using Source = std::function<
			bool(const TextureDesc&, std::vector<std::byte>&, void*)>;

		auto Create(
			const TextureDesc& desc,
			std::span<const std::byte> pixels,
			Source source = {}
		) -> TextureId;

		auto Create(
			const TextureDesc& desc,
			std::vector<std::byte> pixels,
			Source source
		) -> TextureId;

		auto Update(
			TextureId id,
			std::span<const std::byte> pixels,
			std::optional<TextureDesc> desc = std::nullopt
		) -> bool;

		void Destroy(TextureId id);
		void SetSource(TextureId id, Source source, void* user = nullptr);

		[[nodiscard]] auto Get(TextureId id) -> Texture*;
		[[nodiscard]] auto Get(TextureId id) const -> const Texture*;

		[[nodiscard]] auto IsShared(TextureId id) const -> bool;

		void Acquire(TextureId id, Surface* consumer);
		void Release(TextureId id, Surface* consumer);
		void ReleaseConsumerSurface(Surface* surface);

		void SetRetainCPU(TextureId id, bool retain);
		void DropCPU(TextureId id);

		auto Subscribe(std::span<Surface* const> served) -> SubscriberId;
		void Unsubscribe(SubscriberId sub);
		void Acknowledge(SubscriberId sub, TextureId id, uint32_t revision);
		void AcknowledgeRemoved(SubscriberId sub, TextureId id);

		void CollectFor(Surface* surface, std::vector<const Texture*>& out);

		void FlushReleases();

		auto Load(std::span<const std::byte> data, uint32_t flags = 0)
			-> gfx::TextureId;
		auto Load(const std::string& path, uint32_t flags = 0)
			-> gfx::TextureId;

		auto LoadNamed(
			const std::string& key,
			const std::string& path,
			uint32_t flags = 0
		) -> gfx::TextureId;
		auto LoadNamed(
			const std::string& key,
			std::span<const std::byte> data,
			uint32_t flags = 0
		) -> gfx::TextureId;

	private:
		struct Slot
		{
			uint32_t Gen = 1;
			bool Alive = false;
			bool Owned = false;
			bool Destroying = false;
			Texture Data;
			Source Src;
			void* User = nullptr;
			uint32_t HydrateFailedRev = 0;
			std::vector<Surface*> Consumers;
			std::unordered_map<SubscriberId, uint32_t> Acked;
		};

		struct Subscriber
		{
			std::vector<Surface*> Served;
		};

		struct Named
		{
			TextureId Id;
			uint32_t Flags = 0;
		};

		std::vector<Slot> _slots;
		std::vector<uint32_t> _free;
		std::unordered_map<SubscriberId, Subscriber> _subs;
		std::unordered_map<std::string, Named> _named;
		SubscriberId _nextSub = 1;

		auto _slot(TextureId id) -> Slot*;
		auto _anySlot(TextureId id) -> Slot*;
		static void CheckDestroy(Slot& s);
		void _maybeDropCPU(Slot& s);
		auto _needsPixels(const Slot& s, Surface* surface) const -> bool;
		static auto Hydrate(Slot& s) -> bool;
	};
}

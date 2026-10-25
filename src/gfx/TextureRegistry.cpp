#include "gfx/TextureRegistry.h"
#include "core/Logger.h"
#include "gfx/ImageDecoder.h"
#include "gfx/ImageDecoderRegistry.h"
#include <algorithm>

namespace mocca::gfx
{
	auto expectedSize(const TextureDesc& desc) -> std::size_t
	{
		return static_cast<std::size_t>(desc.Width) * desc.Height *
			   bytesPerPixel(desc.Format);
	}

	auto servesAny(
		const std::vector<mocca::Surface*>& served,
		const std::vector<mocca::Surface*>& consumers
	) -> bool
	{
		return std::ranges::any_of(
			served,
			[&consumers](mocca::Surface* s) -> auto
			{ return std::ranges::contains(consumers, s); }
		);
	}

	auto TextureRegistry::_slot(TextureId id) -> Slot*
	{
		if (id.Id >= _slots.size())
		{
			return nullptr;
		}

		auto& s = _slots[id.Id];
		if (!s.Alive || s.Destroying || s.Gen != id.Gen)
		{
			return nullptr;
		}

		return &s;
	}

	auto TextureRegistry::_anySlot(TextureId id) -> Slot*
	{
		if (id.Id >= _slots.size())
		{
			return nullptr;
		}

		auto& s = _slots[id.Id];
		if (!s.Alive || s.Gen != id.Gen)
		{
			return nullptr;
		}

		return &s;
	}

	auto TextureRegistry::Create(
		const TextureDesc& desc,
		std::span<const std::byte> pixels,
		Source source
	) -> TextureId
	{
		auto wanted = expectedSize(desc);
		if (!pixels.empty() && pixels.size() != wanted)
		{
			mc_error(
				ErrorCode::InvalidArgument,
				"texture {}x{} expects {} bytes, got {}",
				desc.Width,
				desc.Height,
				wanted,
				pixels.size()
			);
			return {};
		}

		uint32_t index = 0;
		if (_free.empty())
		{
			index = static_cast<uint32_t>(_slots.size());
			_slots.emplace_back();
		}
		else
		{
			index = _free.back();
			_free.pop_back();
		}

		auto& s = _slots[index];
		s.Alive = true;
		s.Owned = true;
		s.Destroying = false;
		s.Src = std::move(source);
		s.HydrateFailedRev = 0;
		s.Consumers.clear();
		s.Acked.clear();
		s.Data.Id = {.Id = index, .Gen = s.Gen};
		s.Data.Desc = desc;
		s.Data.Revision = 1;
		s.Data.Pixels.assign(pixels.begin(), pixels.end());

		return s.Data.Id;
	}

	auto TextureRegistry::Update(
		TextureId id,
		std::span<const std::byte> pixels,
		std::optional<TextureDesc> desc
	) -> bool
	{
		auto* s = _slot(id);
		if (s == nullptr)
		{
			return false;
		}

		auto next = desc.value_or(s->Data.Desc);
		if (pixels.size() != expectedSize(next))
		{
			mc_error(
				ErrorCode::InvalidArgument,
				"texture {}x{} expects {} bytes, got {}",
				next.Width,
				next.Height,
				expectedSize(next),
				pixels.size()
			);
			return false;
		}

		s->Data.Desc = next;
		s->Data.Pixels.assign(pixels.begin(), pixels.end());
		s->Data.Revision++;
		s->HydrateFailedRev = 0;

		return true;
	}

	void TextureRegistry::Destroy(TextureId id)
	{
		auto* s = _slot(id);
		if (s == nullptr)
		{
			return;
		}

		s->Owned = false;
		CheckDestroy(*s);
	}

	void TextureRegistry::SetSource(TextureId id, Source source, void* user)
	{
		auto* s = _slot(id);
		if (s == nullptr)
		{
			return;
		}

		s->Src = std::move(source);
		s->HydrateFailedRev = 0;
		s->User = user;
	}

	auto TextureRegistry::Get(TextureId id) -> Texture*
	{
		auto* s = _slot(id);
		return s != nullptr ? &s->Data : nullptr;
	}

	auto TextureRegistry::Get(TextureId id) const -> const Texture*
	{
		if (id.Id >= _slots.size())
		{
			return nullptr;
		}

		const auto& s = _slots[id.Id];
		if (!s.Alive || s.Destroying || s.Gen != id.Gen)
		{
			return nullptr;
		}

		return &s.Data;
	}

	auto TextureRegistry::IsShared(TextureId id) const -> bool
	{
		if (id.Id >= _slots.size())
		{
			return false;
		}

		const auto& s = _slots[id.Id];
		if (!s.Alive || s.Destroying || s.Gen != id.Gen)
		{
			return false;
		}

		return (s.Data.Desc.Flags & TextureFlags::Shared) != 0;
	}

	void TextureRegistry::Acquire(TextureId id, Surface* consumer)
	{
		auto* s = _slot(id);
		if (s == nullptr || consumer == nullptr)
		{
			return;
		}

		if (!std::ranges::contains(s->Consumers, consumer))
		{
			s->Consumers.push_back(consumer);
			s->HydrateFailedRev = 0;
		}
	}

	void TextureRegistry::Release(TextureId id, Surface* consumer)
	{
		auto* s = _slot(id);
		if (s == nullptr)
		{
			return;
		}

		std::erase(s->Consumers, consumer);
		CheckDestroy(*s);
		_maybeDropCPU(*s);
	}

	void TextureRegistry::ReleaseConsumerSurface(Surface* surface)
	{
		for (auto& s : _slots)
		{
			if (!s.Alive || std::erase(s.Consumers, surface) == 0)
			{
				continue;
			}

			CheckDestroy(s);
			_maybeDropCPU(s);
		}
	}

	void TextureRegistry::SetRetainCPU(TextureId id, bool retain)
	{
		auto* s = _slot(id);
		if (s == nullptr)
		{
			return;
		}

		if (retain)
		{
			s->Data.Desc.Flags |= TextureFlags::RetainCPU;
			return;
		}

		s->Data.Desc.Flags &= ~TextureFlags::RetainCPU;
		_maybeDropCPU(*s);
	}

	void TextureRegistry::DropCPU(TextureId id)
	{
		auto* s = _slot(id);
		if (s == nullptr)
		{
			return;
		}

		std::vector<std::byte>().swap(s->Data.Pixels);
	}

	auto TextureRegistry::Subscribe(std::span<Surface* const> served)
		-> SubscriberId
	{
		auto id = _nextSub++;
		_subs[id].Served.assign(served.begin(), served.end());
		return id;
	}

	void TextureRegistry::Unsubscribe(SubscriberId sub)
	{
		_subs.erase(sub);

		for (auto& s : _slots)
		{
			if (!s.Alive)
			{
				continue;
			}

			s.Acked.erase(sub);
			_maybeDropCPU(s);
		}
	}

	void TextureRegistry::Acknowledge(
		SubscriberId sub,
		TextureId id,
		uint32_t revision
	)
	{
		auto* s = _anySlot(id);
		if (s == nullptr)
		{
			return;
		}

		s->Acked[sub] = revision;
		_maybeDropCPU(*s);
	}

	void TextureRegistry::AcknowledgeRemoved(SubscriberId sub, TextureId id)
	{
		auto* s = _anySlot(id);
		if (s == nullptr)
		{
			return;
		}

		s->Acked.erase(sub);
	}

	void TextureRegistry::CollectFor(
		Surface* surface,
		std::vector<const Texture*>& out
	)
	{
		for (auto& s : _slots)
		{
			if (!s.Alive || s.Destroying)
			{
				continue;
			}

			bool shared = (s.Data.Desc.Flags & TextureFlags::Shared) != 0;
			if (!shared && !std::ranges::contains(s.Consumers, surface))
			{
				continue;
			}

			if (_needsPixels(s, surface))
			{
				Hydrate(s);
			}

			out.push_back(&s.Data);
		}
	}

	void TextureRegistry::FlushReleases()
	{
		for (uint32_t i = 0; i < _slots.size(); i++)
		{
			auto& s = _slots[i];
			if (!s.Alive || !s.Destroying || !s.Acked.empty())
			{
				continue;
			}

			s.Alive = false;
			s.Owned = false;
			s.Destroying = false;
			s.Src = {};
			s.HydrateFailedRev = 0;
			s.Consumers.clear();
			s.Acked.clear();
			s.Data = {};
			s.Gen++;
			_free.push_back(i);
		}
	}

	void TextureRegistry::CheckDestroy(Slot& s)
	{
		if (s.Destroying || s.Owned)
		{
			return;
		}

		bool shared = (s.Data.Desc.Flags & TextureFlags::Shared) != 0;
		if (!shared && !s.Consumers.empty())
		{
			return;
		}

		s.Destroying = true;
	}

	void TextureRegistry::_maybeDropCPU(Slot& s)
	{
		if (!s.Alive || s.Destroying || s.Data.Pixels.empty())
		{
			return;
		}

		if ((s.Data.Desc.Flags & TextureFlags::RetainCPU) != 0)
		{
			return;
		}

		bool shared = (s.Data.Desc.Flags & TextureFlags::Shared) != 0;
		bool relevant = false;

		for (auto& [Sub, subscriber] : _subs)
		{
			if (!shared && !servesAny(subscriber.Served, s.Consumers))
			{
				continue;
			}

			relevant = true;

			auto ack = s.Acked.find(Sub);
			if (ack == s.Acked.end() || ack->second != s.Data.Revision)
			{
				return;
			}
		}

		if (!relevant)
		{
			return;
		}

		std::vector<std::byte>().swap(s.Data.Pixels);
	}

	auto TextureRegistry::_needsPixels(const Slot& s, Surface* surface) const
		-> bool
	{
		if (!s.Data.Pixels.empty() || s.Src == nullptr)
		{
			return false;
		}

		for (const auto& [Sub, Subscriber] : _subs)
		{
			if (!std::ranges::contains(Subscriber.Served, surface))
			{
				continue;
			}

			auto ack = s.Acked.find(Sub);
			if (ack == s.Acked.end() || ack->second != s.Data.Revision)
			{
				return true;
			}
		}

		return false;
	}

	auto TextureRegistry::Hydrate(Slot& s) -> bool
	{
		if (s.Src == nullptr || s.HydrateFailedRev == s.Data.Revision)
		{
			return false;
		}

		auto wanted = expectedSize(s.Data.Desc);
		std::vector<std::byte> pixels(wanted);

		if (!s.Src(s.Data.Desc, pixels, s.User) || pixels.size() != wanted)
		{
			s.HydrateFailedRev = s.Data.Revision;
			mc_warning(
				"texture source failed to regenerate {}x{} rev {}",
				s.Data.Desc.Width,
				s.Data.Desc.Height,
				s.Data.Revision
			);
			return false;
		}

		s.Data.Pixels = std::move(pixels);
		return true;
	}

	auto TextureRegistry::Load(
		std::span<const std::byte> data,
		uint32_t flags
	) -> gfx::TextureId
	{
		gfx::DecodedImage image;
		if (!ImageDecoderRegistry::Decode(data, image))
		{
			return {};
		}

		gfx::TextureDesc desc{
			.Width = image.Width,
			.Height = image.Height,
			.Format = image.Format,
			.Flags = flags,
		};

		std::vector<std::byte> bytes(data.begin(), data.end());
		auto source = [bytes = std::move(bytes)](
						  const gfx::TextureDesc& wanted,
						  std::vector<std::byte>& out,
						  void*
					  ) -> bool
		{
			gfx::DecodedImage decoded;
			if (!ImageDecoderRegistry::Decode(bytes, decoded))
			{
				return false;
			}
			if (decoded.Width != wanted.Width ||
				decoded.Height != wanted.Height)
			{
				return false;
			}
			out = std::move(decoded.Pixels);
			return true;
		};

		auto id = Create(desc, image.Pixels, source);
		mc_debug("created texture {} (gen {}) from data ({} bytes)", id.Id, id.Gen, bytes.size());

		return id;
	}

	auto TextureRegistry::Load(const std::string& path, uint32_t flags)
		-> gfx::TextureId
	{
		gfx::DecodedImage image;
		if (!ImageDecoderRegistry::DecodeFile(path, image))
		{
			return {};
		}

		gfx::TextureDesc desc{
			.Width = image.Width,
			.Height = image.Height,
			.Format = image.Format,
			.Flags = flags,
		};

		auto source = [path](
						  const gfx::TextureDesc& wanted,
						  std::vector<std::byte>& out,
						  void*
					  ) -> bool
		{
			gfx::DecodedImage decoded;
			if (!ImageDecoderRegistry::DecodeFile(path, decoded))
			{
				return false;
			}
			if (decoded.Width != wanted.Width ||
				decoded.Height != wanted.Height)
			{
				return false;
			}
			out = std::move(decoded.Pixels);
			return true;
		};

		auto id = Create(desc, image.Pixels, source);
		mc_debug(
			"created texture {} (gen {}) from file {}",
			id.Id,
			id.Gen,
			path
		);

		return id;
	}
}

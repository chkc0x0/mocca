#include "core/HookStore.h"

namespace mocca::detail
{
	auto HookStore::Has(NodeId id, std::uint32_t hook) -> bool
	{
		HookKey key{.Id = id, .Hook = hook};
		return _slots.contains(key);
	}

	auto HookStore::GetEffectSlot(NodeId id, std::uint32_t hook) -> EffectSlot&
	{
		HookKey key{.Id = id, .Hook = hook};
		auto it = _slots.find(key);
		if (it == _slots.end())
		{
			it = _slots.emplace(key, std::any(EffectSlot{})).first;
		}
		return std::any_cast<EffectSlot&>(it->second);
	}

	auto HookStore::GetMemoSlot(NodeId id, std::uint32_t hook) -> MemoSlot&
	{
		HookKey key{.Id = id, .Hook = hook};
		auto it = _slots.find(key);
		if (it == _slots.end())
		{
			it = _slots.emplace(key, std::any(MemoSlot{})).first;
		}
		return std::any_cast<MemoSlot&>(it->second);
	}

	void HookStore::FlushEffects()
	{
		auto q = std::move(_effectQueue);
		_effectQueue.clear();
		for (auto& effect : q)
		{
			try
			{
				effect.Callback();
			}
			catch (const std::exception& e)
			{
				mc_error(
					ErrorCode::UserSide,
					"effect threw an exception: {}",
					e.what()
				);
			}
			catch (...)
			{
				mc_error(
					ErrorCode::UserSide,
					"effect threw a non-std exception"
				);
			}
		}
	}

	void HookStore::FlushLayoutEffects(Surface* surface)
	{
		std::vector<EffectFunction> kept;
		kept.reserve(_effectQueue.size());
		for (auto& e : _effectQueue)
		{
			if (e.IsLayout &&
				(surface == nullptr || e.OwningSurface == surface))
			{
				try
				{
					e.Callback();
				}
				catch (const std::exception& ex)
				{
					mc_error(
						ErrorCode::UserSide,
						"layout effect threw an exception: {}",
						ex.what()
					);
				}
				catch (...)
				{
					mc_error(
						ErrorCode::UserSide,
						"layout effect threw a non-std exception"
					);
				}
			}
			else
			{
				kept.push_back(std::move(e));
			}
		}
		_effectQueue = std::move(kept);
	}

	void HookStore::RemoveComponent(NodeId id)
	{
		for (auto it = _slots.begin(); it != _slots.end();)
		{
			if (it->first.Id == id)
			{
				if (auto* eff = std::any_cast<EffectSlot>(&it->second))
				{
					auto old = std::move(eff->Cleanup);
					eff->Cleanup = nullptr;
					if (old)
					{
						try
						{
							old();
						}
						catch (const std::exception& e)
						{
							mc_error(
								ErrorCode::UserSide,
								"cleanup threw during removal: {}",
								e.what()
							);
						}
						catch (...)
						{
							mc_error(
								ErrorCode::UserSide,
								"cleanup threw a non-std exception during "
								"removal"
							);
						}
					}
				}
				it = _slots.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	void HookStore::PushEffect(const CleanupFn& fn)
	{
		_effectQueue.push_back({.Callback = fn});
	}

	void HookStore::PushLayoutEffect(const CleanupFn& fn, Surface* surface)
	{
		_effectQueue.push_back({
			.Callback = fn,
			.IsLayout = true,
			.OwningSurface = surface,
		});
	}

	void HookStore::SetMarkDirty(std::function<void(NodeId)> fn)
	{
		_markDirty = std::move(fn);
	}

	void HookStore::ClearDirty()
	{
		_dirtySet.clear();
	}

	void HookStore::InsertDirty(NodeId id)
	{
		_dirtySet.insert(id);
		_lastDirty = id;
	}

	auto HookStore::DirtyCount() const -> size_t
	{
		return _dirtySet.size();
	}

	auto HookStore::LastDirty() const -> uint64_t
	{
		return _lastDirty;
	}
}
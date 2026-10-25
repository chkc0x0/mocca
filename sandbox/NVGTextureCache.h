#pragma once
#include "gfx/TextureCache.h"
#include "nanovg.h"
#include <span>

namespace sandbox
{
	class NVGTextureCache : public mocca::gfx::TextureCache
	{
	public:
		NVGTextureCache(
			mocca::gfx::TextureRegistry& registry,
			NVGcontext* vg,
			std::span<mocca::Surface* const> served
		)
			: TextureCache(registry, served), _vg(vg)
		{
		}

		~NVGTextureCache() override
		{
			Shutdown();
		}

	protected:
		auto CreateImpl(const mocca::gfx::Texture& src) -> uintptr_t override
		{
			if (src.Desc.Format != mocca::gfx::TextureFormat::RGBA8)
			{
				return 0;
			}

			return static_cast<uintptr_t>(nvgCreateImageRGBA(
				_vg,
				static_cast<int>(src.Desc.Width),
				static_cast<int>(src.Desc.Height),
				0,
				reinterpret_cast<const unsigned char*>(src.Pixels.data())
			));
		}

		void UpdateImpl(uintptr_t handle, const mocca::gfx::Texture& src) override
		{
			nvgUpdateImage(
				_vg,
				static_cast<int>(handle),
				reinterpret_cast<const unsigned char*>(src.Pixels.data())
			);
		}

		void DestroyImpl(uintptr_t handle) override
		{
			nvgDeleteImage(_vg, static_cast<int>(handle));
		}

	private:
		NVGcontext* _vg;
	};
}

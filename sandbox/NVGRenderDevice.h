#pragma once
#include "gfx/RenderDevice.h"
#include "text/FontTypes.h"
#include <unordered_map>

struct NVGcontext;

namespace sandbox
{
	class NVGRenderDevice : public mocca::gfx::RenderDevice
	{
	public:
		~NVGRenderDevice() override;

		[[nodiscard]] auto Requirements() const
			-> mocca::gfx::GraphicsRequirements override;

		auto CreateRenderer(
			mocca::Surface* surf,
			const mocca::gfx::GraphicsHandle& handle
		) -> std::unique_ptr<mocca::gfx::Renderer> override;
	};

	class NVGRenderer : public mocca::gfx::Renderer
	{
	public:
		explicit NVGRenderer(const mocca::gfx::GraphicsHandle& handle);
		~NVGRenderer() override;

		void Render(const mocca::gfx::FrameInfo& frame) override;

	private:
		auto _fontIdFor(mocca::mtext::FontFaceId id) -> int;

		NVGcontext* _vg = nullptr;
		std::unordered_map<mocca::mtext::FontFaceId, int> _fonts;
	};
}

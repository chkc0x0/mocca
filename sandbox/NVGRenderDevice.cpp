#include "NVGRenderDevice.h"
#include "core/Application.h"
#include "core/Logger.h"
#include "text/FontRegistry.h"
#include "text/TextLayout.h"
#include <algorithm>
#include <cstdlib>
#include <string>
#include <type_traits>
#include "GL/glew.h"
#include "nanovg.h"
#include <span>
#include <utility>
#define NANOVG_GL3_IMPLEMENTATION 1
#include "nanovg_gl.h"

namespace sandbox
{
	constexpr bool DebugRunBoxes = true;

	NVGRenderDevice::~NVGRenderDevice() = default;

	auto NVGRenderDevice::Requirements() const
		-> mocca::gfx::GraphicsRequirements
	{
		return {
			.Api = mocca::gfx::GraphicsApi::OpenGL,
			.Major = 3,
			.Minor = 3,
			.Flags = mocca::gfx::GraphicsFlags::Core,
		};
	}

	auto NVGRenderDevice::CreateRenderer(
		mocca::Surface* surf,
		const mocca::gfx::GraphicsHandle& handle
	) -> std::unique_ptr<mocca::gfx::Renderer>
	{
		return std::make_unique<NVGRenderer>(surf, handle);
	}

	NVGRenderer::NVGRenderer(
		mocca::Surface* surface,
		const mocca::gfx::GraphicsHandle& handle
	)
	{
		(void)handle;

		_vg = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
		if (_vg == nullptr)
		{
			mc_error(
				mocca::ErrorCode::InvalidState,
				"failed to create NanoVG context"
			);
			return;
		}

		_texCache = std::make_unique<NVGTextureCache>(
			mocca::Application::main->Textures(),
			_vg,
			std::span<mocca::Surface* const>(&surface, 1)
		);
	}

	NVGRenderer::~NVGRenderer()
	{
		_texCache.reset();

		if (_vg != nullptr)
		{
			nvgDeleteGL3(_vg);
			_vg = nullptr;
		}
	}

	void NVGRenderer::Render(const mocca::gfx::FrameInfo& frame)
	{
		if (_vg == nullptr)
		{
			return;
		}

		if (_texCache != nullptr)
		{
			_texCache->Sync(frame.Textures);
		}

		nvgBeginFrame(
			_vg,
			frame.ViewportSize.X,
			frame.ViewportSize.Y,
			frame.Scale.X
		);
		nvgResetScissor(_vg);

		for (const auto& cmd : frame.Commands)
		{
			std::visit(
				[this](const auto& c) -> void
				{
					using T = std::decay_t<decltype(c)>;
					if constexpr (std::is_same_v<T, mocca::cmds::DrawRectCmd>)
					{
						const auto& radius = c.Radius;
						bool rounded = radius.TopLeft > 0 ||
									   radius.TopRight > 0 ||
									   radius.BottomRight > 0 ||
									   radius.BottomLeft > 0;

						nvgBeginPath(_vg);
						if (rounded)
						{
							nvgRoundedRectVarying(
								_vg,
								c.Rect.X,
								c.Rect.Y,
								c.Rect.Width,
								c.Rect.Height,
								radius.TopLeft,
								radius.TopRight,
								radius.BottomRight,
								radius.BottomLeft
							);
						}
						else
						{
							nvgRect(
								_vg,
								c.Rect.X,
								c.Rect.Y,
								c.Rect.Width,
								c.Rect.Height
							);
						}
						nvgFillColor(
							_vg,
							nvgRGBA(c.Color.R, c.Color.G, c.Color.B, c.Color.A)
						);
						nvgFill(_vg);

						float borderWidth = std::max({
							c.Border.Top,
							c.Border.Right,
							c.Border.Bottom,
							c.Border.Left,
						});
						if (borderWidth > 0 && c.BorderColor.A > 0)
						{
							nvgStrokeColor(
								_vg,
								nvgRGBA(
									c.BorderColor.R,
									c.BorderColor.G,
									c.BorderColor.B,
									c.BorderColor.A
								)
							);
							nvgStrokeWidth(_vg, borderWidth);
							nvgStroke(_vg);
						}
					}
					else if constexpr (
						std::is_same_v<T, mocca::cmds::DrawTextCmd>
					)
					{
						if (c.Layout == nullptr)
						{
							return;
						}

						nvgTextLetterSpacing(_vg, c.Layout->LetterSpacing);
						nvgTextAlign(_vg, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);
						nvgFillColor(
							_vg,
							nvgRGBA(c.Color.R, c.Color.G, c.Color.B, c.Color.A)
						);

						for (const auto& line : c.Layout->Lines)
						{
							float lineX = c.Position.X +
										  line.XOffset(c.BoxWidth, c.Align);
							for (uint32_t r = line.RunBegin; r < line.RunEnd;
								 r++)
							{
								const auto& run = c.Layout->Runs[r];
								int font = _fontIdFor(run.Face);
								if (font < 0)
								{
									continue;
								}

								nvgFontFaceId(_vg, font);
								nvgFontSize(_vg, run.Size);
								nvgText(
									_vg,
									lineX + run.X,
									c.Position.Y + line.Baseline,
									c.Content.data() + run.Start,
									c.Content.data() + run.End
								);
							}
						}

						if constexpr (DebugRunBoxes)
						{
							float halfLeading = (c.Layout->LineHeight -
												 (c.Layout->Ascent +
												  c.Layout->Descent)) *
												0.5F;
							for (const auto& line : c.Layout->Lines)
							{
								float left = c.Position.X +
											 line.XOffset(c.BoxWidth, c.Align);
								float emTop = c.Position.Y + line.Baseline -
											  c.Layout->Ascent;

								for (uint32_t r = line.RunBegin;
									 r < line.RunEnd;
									 r++)
								{
									const auto& run = c.Layout->Runs[r];
									nvgBeginPath(_vg);
									nvgRect(
										_vg,
										left + run.X,
										emTop - halfLeading,
										run.Width,
										c.Layout->LineHeight
									);
									nvgFillColor(
										_vg,
										nvgRGBA(
											static_cast<unsigned char>(
												rand() % 256
											),
											static_cast<unsigned char>(
												rand() % 256
											),
											static_cast<unsigned char>(
												rand() % 256
											),
											110
										)
									);
									nvgFill(_vg);
								}
							}
						}
					}
					else if constexpr (
						std::is_same_v<T, mocca::cmds::PushClipCmd>
					)
					{
						nvgSave(_vg);
						nvgScissor(
							_vg,
							c.Rect.X,
							c.Rect.Y,
							c.Rect.Width,
							c.Rect.Height
						);
					}
					else if constexpr (
						std::is_same_v<T, mocca::cmds::PopClipCmd> ||
						std::is_same_v<T, mocca::cmds::PopTransformCmd>
					)
					{
						nvgRestore(_vg);
					}
					else if constexpr (
						std::is_same_v<T, mocca::cmds::PushTransformCmd>
					)
					{
						nvgSave(_vg);
						nvgTranslate(_vg, c.Offset.X, c.Offset.Y);
					}
				},
				cmd
			);
		}

		nvgEndFrame(_vg);
	}

	auto NVGRenderer::_fontIdFor(mocca::mtext::FontFaceId id) -> int
	{
		auto found = _fonts.find(id);
		if (found != _fonts.end())
		{
			return found->second;
		}

		int handle = -1;
		if (_vg != nullptr && mocca::Application::main != nullptr &&
			std::cmp_not_equal(id ,-1))
		{
			const auto& entry = mocca::Application::main->Fonts().Entry(id);
			std::string name = "mocca-face-" + std::to_string(id);

			if (const auto* path = std::get_if<std::string>(&entry.Source))
			{
				handle = nvgCreateFont(_vg, name.c_str(), path->c_str());
			}
			else if (
				const auto* memory =
					std::get_if<mocca::mtext::FontMemorySource>(&entry.Source)
			)
			{
				handle = nvgCreateFontMem(
					_vg,
					name.c_str(),
					reinterpret_cast<unsigned char*>(
						const_cast<std::byte*>(memory->Data.data())
					),
					static_cast<int>(memory->Data.size()),
					0
				);
			}
		}

		_fonts.emplace(id, handle);
		return handle;
	}
}

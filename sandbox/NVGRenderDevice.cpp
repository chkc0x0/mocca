#include "NVGRenderDevice.h"
#include "core/Application.h"
#include "core/Logger.h"
#include "gfx/Paint.h"
#include "text/FontRegistry.h"
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
	constexpr bool DebugRunBoxes = false;

	auto hasRadius(const mocca::CornerRadii& radii) -> bool
	{
		return radii.TopLeft > 0 || radii.TopRight > 0 ||
			   radii.BottomRight > 0 || radii.BottomLeft > 0;
	}

	auto addRRect(NVGcontext* vg, const mocca::RRect& rrect) -> void
	{
		const auto& rect = rrect.Rect;
		if (hasRadius(rrect.Radii))
		{
			nvgRoundedRectVarying(
				vg,
				rect.X,
				rect.Y,
				rect.Width,
				rect.Height,
				rrect.Radii.TopLeft,
				rrect.Radii.TopRight,
				rrect.Radii.BottomRight,
				rrect.Radii.BottomLeft
			);
		}
		else
		{
			nvgRect(vg, rect.X, rect.Y, rect.Width, rect.Height);
		}
	}

	auto strokeCapOf(mocca::gfx::StrokeCap cap) -> int
	{
		switch (cap)
		{
		case mocca::gfx::StrokeCap::Round:
			return NVG_ROUND;
		case mocca::gfx::StrokeCap::Square:
			return NVG_SQUARE;
		case mocca::gfx::StrokeCap::Butt:
		default:
			return NVG_BUTT;
		}
	}

	auto strokeJoinOf(mocca::gfx::StrokeJoin join) -> int
	{
		switch (join)
		{
		case mocca::gfx::StrokeJoin::Round:
			return NVG_ROUND;
		case mocca::gfx::StrokeJoin::Bevel:
			return NVG_BEVEL;
		case mocca::gfx::StrokeJoin::Miter:
		default:
			return NVG_MITER;
		}
	}

	auto toNvgColor(mocca::Color color) -> NVGcolor
	{
		return nvgRGBA(color.R, color.G, color.B, color.A);
	}

	auto applyPaint(NVGcontext* vg, const mocca::gfx::Paint& paint) -> void
	{
		nvgGlobalAlpha(vg, paint.Opacity);
		nvgFillColor(vg, toNvgColor(paint.Color));
		nvgStrokeColor(vg, toNvgColor(paint.Color));
		nvgStrokeWidth(vg, paint.StrokeWidth);
		nvgLineCap(vg, strokeCapOf(paint.Cap));
		nvgLineJoin(vg, strokeJoinOf(paint.Join));
		nvgMiterLimit(vg, paint.MiterLimit);

		switch (paint.Style)
		{
		case mocca::gfx::PaintStyle::Stroke:
			nvgStroke(vg);
			break;
		case mocca::gfx::PaintStyle::FillAndStroke:
			nvgFill(vg);
			nvgStroke(vg);
			break;
		case mocca::gfx::PaintStyle::Fill:
		default:
			nvgFill(vg);
			break;
		}
	}

	template <typename Build>
	auto
	drawShape(NVGcontext* vg, const mocca::gfx::Paint& paint, Build&& buildPath)
		-> void
	{
		nvgBeginPath(vg);
		buildPath();
		applyPaint(vg, paint);
	}

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

	auto NVGRenderer::Capabilities() const -> const mocca::gfx::Capabilities&
	{
		using mocca::gfx::bits;
		using mocca::gfx::Feature;

		static const mocca::gfx::Capabilities Caps{
			.Features = bits(Feature::StrokedPaths) |
						bits(Feature::RoundedRects) | bits(Feature::DRRects) |
						bits(Feature::ClipRect) | bits(Feature::ImageSampling) |
						bits(Feature::ImageRounded) |
						bits(Feature::Text) | bits(Feature::Opacity),
			.MaxTextureSize = 4096,
			.MaxPathPoints = 0,
			.TextureFormats =
				1U << static_cast<uint32_t>(mocca::gfx::TextureFormat::RGBA8),
		};
		return Caps;
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
						drawShape(
							_vg,
							c.Paint,
							[&]() -> void
							{
								nvgRect(
									_vg,
									c.Rect.X,
									c.Rect.Y,
									c.Rect.Width,
									c.Rect.Height
								);
							}
						);
					}
					else if constexpr (
						std::is_same_v<T, mocca::cmds::DrawRRectCmd>
					)
					{
						drawShape(
							_vg,
							c.Paint,
							[&]() -> void { addRRect(_vg, c.Rect); }
						);
					}
					else if constexpr (
						std::is_same_v<T, mocca::cmds::DrawDRRectCmd>
					)
					{
						nvgBeginPath(_vg);
						addRRect(_vg, c.Outer);
						if (!c.Inner.IsEmpty())
						{
							addRRect(_vg, c.Inner);
							nvgPathWinding(_vg, NVG_HOLE);
						}
						applyPaint(_vg, c.Paint);
					}
					else if constexpr (
						std::is_same_v<T, mocca::cmds::DrawImageCmd>
					)
					{
						if (_texCache == nullptr)
						{
							return;
						}

						auto handle = _texCache->Handle(c.Texture);
						if (handle == 0)
						{
							return;
						}

						nvgBeginPath(_vg);
						addRRect(_vg, c.Dst);
						nvgFillPaint(
							_vg,
							nvgImagePattern(
								_vg,
								c.Dst.Rect.X,
								c.Dst.Rect.Y,
								c.Dst.Rect.Width,
								c.Dst.Rect.Height,
								0.0F,
								static_cast<int>(handle),
								c.Paint.Opacity
							)
						);
						nvgFill(_vg);
					}
					else if constexpr (
						std::is_same_v<T, mocca::cmds::DrawTextCmd>
					)
					{
						if (c.Layout == nullptr)
						{
							return;
						}

						nvgGlobalAlpha(_vg, c.Paint.Opacity);
						nvgTextLetterSpacing(_vg, 0.0F);
						nvgTextAlign(_vg, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);
						nvgFillColor(_vg, toNvgColor(c.Paint.Color));

						for (const auto& line : c.Layout->Lines)
						{
							float lineX = c.Position.X +
										  line.XOffset(c.BoxWidth, c.Align);
							float lineY = c.Position.Y + line.Baseline;
							for (uint32_t ci = line.ClusterBegin;
								 ci < line.ClusterEnd;
								 ci++)
							{
								const auto& cluster = c.Layout->Clusters[ci];
								if (cluster.End <= cluster.Start)
								{
									continue;
								}

								int font = _fontIdFor(cluster.Face);
								if (font < 0)
								{
									continue;
								}

								nvgFontFaceId(_vg, font);
								nvgFontSize(_vg, c.Layout->FontSize);
								nvgText(
									_vg,
									lineX + cluster.X,
									lineY,
									c.Content.data() + cluster.Start,
									c.Content.data() + cluster.End
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
					else if constexpr (std::is_same_v<T, mocca::cmds::SaveCmd>)
					{
						nvgSave(_vg);
					}
					else if constexpr (
						std::is_same_v<T, mocca::cmds::RestoreCmd>
					)
					{
						nvgRestore(_vg);
					}
					else if constexpr (
						std::is_same_v<T, mocca::cmds::ConcatCmd>
					)
					{
						const auto& m = c.Matrix.M;
						nvgTransform(_vg, m[0], m[3], m[1], m[4], m[2], m[5]);
					}
					else if constexpr (
						std::is_same_v<T, mocca::cmds::ClipRectCmd>
					)
					{
						if (c.Op == mocca::gfx::ClipOp::Intersect)
						{
							nvgScissor(
								_vg,
								c.Rect.X,
								c.Rect.Y,
								c.Rect.Width,
								c.Rect.Height
							);
						}
					}
					else if constexpr (
						std::is_same_v<T, mocca::cmds::ClipRRectCmd>
					)
					{
						if (c.Op == mocca::gfx::ClipOp::Intersect)
						{
							const auto& rect = c.Rect.Rect;
							nvgScissor(
								_vg,
								rect.X,
								rect.Y,
								rect.Width,
								rect.Height
							);
						}
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
			std::cmp_not_equal(id, -1))
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

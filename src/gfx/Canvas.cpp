#include "gfx/Canvas.h"
#include "core/Logger.h"
#include "gfx/Capabilities.h"

namespace mocca
{
	constexpr float Boundary = 1.0e9F;

	auto boundlessClip() -> Rectangle
	{
		return {
			.X = -Boundary,
			.Y = -Boundary,
			.Width = Boundary * 2.0F,
			.Height = Boundary * 2.0F,
		};
	}

	Canvas::Canvas()
	{
		_matrixStack.push_back(gfx::Matrix::Identity());
		_clipStack.push_back(boundlessClip());
	}

	void Canvas::Save()
	{
		_commands.emplace_back(cmds::SaveCmd{});
		_matrixStack.push_back(_matrixStack.back());
		_clipStack.push_back(_clipStack.back());
	}

	void Canvas::Restore()
	{
		if (_matrixStack.size() > 1)
		{
			_matrixStack.pop_back();
			_clipStack.pop_back();
		}
		_commands.emplace_back(cmds::RestoreCmd{});
	}

	void Canvas::Translate(float dx, float dy)
	{
		Concat(gfx::Matrix::Translate(dx, dy));
	}

	void Canvas::Scale(float sx, float sy)
	{
		Concat(gfx::Matrix::Scale(sx, sy));
	}

	void Canvas::Rotate(float degrees)
	{
		Concat(gfx::Matrix::Rotate(degrees));
	}

	void Canvas::Concat(const gfx::Matrix& matrix)
	{
		_commands.emplace_back(cmds::ConcatCmd{.Matrix = matrix});
		_matrixStack.back() = gfx::Matrix::Concat(_matrixStack.back(), matrix);
	}

	void Canvas::ClipRect(const Rectangle& rect, gfx::ClipOp op)
	{
		_commands.emplace_back(cmds::ClipRectCmd{.Rect = rect, .Op = op});

		if (op == gfx::ClipOp::Intersect)
		{
			_clipStack.back() = _clipStack.back().Intersect(
				_matrixStack.back().MapRect(rect)
			);
		}
	}

	void Canvas::ClipRRect(const RRect& rrect, gfx::ClipOp op)
	{
		_commands.emplace_back(cmds::ClipRRectCmd{.Rect = rrect, .Op = op});

		if (op == gfx::ClipOp::Intersect)
		{
			_clipStack.back() = _clipStack.back().Intersect(
				_matrixStack.back().MapRect(rrect.Rect)
			);
		}
	}

	void Canvas::DrawRect(const Rectangle& rect, const gfx::Paint& paint)
	{
		_commands.emplace_back(cmds::DrawRectCmd{.Rect = rect, .Paint = paint});
	}

	void Canvas::DrawRRect(const RRect& rrect, const gfx::Paint& paint)
	{
		_commands.emplace_back(
			cmds::DrawRRectCmd{.Rect = rrect, .Paint = paint}
		);
	}

	void Canvas::DrawDRRect(
		const RRect& outer,
		const RRect& inner,
		const gfx::Paint& paint
	)
	{
		_commands.emplace_back(
			cmds::DrawDRRectCmd{
				.Outer = outer,
				.Inner = inner,
				.Paint = paint,
			}
		);
	}

	void Canvas::DrawImage(
		const gfx::TextureId& texture,
		const RRect& dst,
		const gfx::Paint& paint,
		const gfx::Sampling& sampling
	)
	{
		_commands.emplace_back(
			cmds::DrawImageCmd{
				.Dst = dst,
				.Texture = texture,
				.Paint = paint,
				.Sampling = sampling,
			}
		);
	}

	void Canvas::DrawImage(
		const gfx::TextureId& texture,
		const Rectangle& dst,
		const gfx::Paint& paint,
		const gfx::Sampling& sampling
	)
	{
		DrawImage(
			texture,
			RRect{.Rect = dst, .Radii = {}},
			paint,
			sampling
		);
	}

	void Canvas::DrawText(cmds::DrawTextCmd cmd)
	{
		_commands.emplace_back(std::move(cmd));
	}

	auto Canvas::QuickReject(const Rectangle& localRect) const -> bool
	{
		if (localRect.IsEmpty())
		{
			return true;
		}

		Rectangle device = _matrixStack.back().MapRect(localRect);
		return !device.Intersects(_clipStack.back());
	}

	auto Canvas::Commands() const -> std::span<const cmds::DrawCommand>
	{
		return _commands;
	}

	void Canvas::Clear()
	{
		_commands.clear();
		_matrixStack.clear();
		_clipStack.clear();
		_matrixStack.push_back(gfx::Matrix::Identity());
		_clipStack.push_back(boundlessClip());
	}

	void Canvas::Append(const Canvas& other)
	{
		_commands.insert(
			_commands.end(),
			other._commands.begin(),
			other._commands.end()
		);
	}

	// i dont want a new file so
	auto gfx::detail::reportMissingFeatures(
		std::span<const cmds::DrawCommand> commands,
		const gfx::Capabilities& caps,
		uint64_t& reported
	) -> uint64_t
	{
#ifdef DEBUG
		uint64_t missing = gfx::drawCmdMissingFeatures(commands, caps) &
								 ~reported;
		if (missing == 0)
		{
			return 0;
		}

		reported |= missing;
		mc_warning(
			"renderer is missing draw features required by this frame "
			"(missing feature bits 0x{:x})",
			missing
		);
		return missing;
#else
		(void)commands;
		(void)caps;
		(void)reported;
		return 0;
#endif
	}
}
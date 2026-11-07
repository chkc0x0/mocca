#pragma once
#include "DrawCommands.h"
#include <span>
#include <vector>

namespace mocca
{
	class Canvas
	{
	public:
		Canvas();

		void Save();
		void Restore();
		void Translate(float dx, float dy);
		void Scale(float sx, float sy);
		void Rotate(float degrees);
		void Concat(const gfx::Matrix& matrix);

		void ClipRect(
			const Rectangle& rect,
			gfx::ClipOp op = gfx::ClipOp::Intersect
		);
		void ClipRRect(
			const RRect& rrect,
			gfx::ClipOp op = gfx::ClipOp::Intersect
		);

		void DrawRect(const Rectangle& rect, const gfx::Paint& paint);
		void DrawRRect(const RRect& rrect, const gfx::Paint& paint);
		void DrawDRRect(
			const RRect& outer,
			const RRect& inner,
			const gfx::Paint& paint
		);
		void DrawImage(
			const gfx::TextureId& texture,
			const RRect& dst,
			const gfx::Paint& paint = {},
			const gfx::Sampling& sampling = {}
		);
		void DrawImage(
			const gfx::TextureId& texture,
			const Rectangle& dst,
			const gfx::Paint& paint = {},
			const gfx::Sampling& sampling = {}
		);
		void DrawText(cmds::DrawTextCmd cmd);

		[[nodiscard]] auto QuickReject(const Rectangle& localRect) const
			-> bool;
		[[nodiscard]] auto Commands() const
			-> std::span<const cmds::DrawCommand>;
		void Clear();
		void Append(const Canvas& other);

	private:
		std::vector<cmds::DrawCommand> _commands;
		std::vector<gfx::Matrix> _matrixStack;
		std::vector<Rectangle> _clipStack;
	};
}

#include "TestSupport.h"
#include "gfx/Canvas.h"
#include "gfx/Matrix.h"
#include <cmath>

namespace mtest
{
	TEST(PaintTest, FillStrokeSetStyle)
	{
		auto fill = mocca::gfx::Paint::Fill(mocca::colors::Red);
		EXPECT_EQ(fill.Style, mocca::gfx::PaintStyle::Fill);
		EXPECT_EQ(fill.Color, mocca::colors::Red);

		auto stroke =
			mocca::gfx::Paint::Stroke(mocca::colors::Blue, 4.0F);
		EXPECT_EQ(stroke.Style, mocca::gfx::PaintStyle::Stroke);
		EXPECT_EQ(stroke.Color, mocca::colors::Blue);
		EXPECT_FLOAT_EQ(stroke.StrokeWidth, 4.0F);
	}

	TEST(PaintTest, CanvasPutsPaintOnCommand)
	{
		mocca::Canvas canvas;
		canvas.DrawRect(
			{.X = 0, .Y = 0, .Width = 10, .Height = 10},
			mocca::gfx::Paint::Fill(mocca::colors::Red)
		);
		canvas.DrawRRect(
			{.Rect = {.X = 0, .Y = 0, .Width = 10, .Height = 10}, .Radii = {}},
			mocca::gfx::Paint::Stroke(mocca::colors::Blue, 2.0F)
		);

		auto cmds = canvas.Commands();
		ASSERT_EQ(cmds.size(), 2U);

		const auto* rect = std::get_if<mocca::cmds::DrawRectCmd>(cmds.data());
		ASSERT_NE(rect, nullptr);
		EXPECT_EQ(rect->Paint.Style, mocca::gfx::PaintStyle::Fill);

		const auto* rrect = std::get_if<mocca::cmds::DrawRRectCmd>(&cmds[1]);
		ASSERT_NE(rrect, nullptr);
		EXPECT_EQ(rrect->Paint.Style, mocca::gfx::PaintStyle::Stroke);
		EXPECT_FLOAT_EQ(rrect->Paint.StrokeWidth, 2.0F);
	}

	TEST(MatrixTest, ConcatRTL)
	{
		auto translate = mocca::gfx::Matrix::Translate(10.0F, 20.0F);
		auto scale = mocca::gfx::Matrix::Scale(2.0F, 3.0F);

		auto combined = mocca::gfx::Matrix::Concat(translate, scale);
		auto mapped = combined.MapPoint({.X = 1.0F, .Y = 1.0F});

		EXPECT_FLOAT_EQ(mapped.X, 12.0F);
		EXPECT_FLOAT_EQ(mapped.Y, 23.0F);
	}

	TEST(MatrixTest, InvertRoundTrips)
	{
		auto matrix = mocca::gfx::Matrix::Concat(
			mocca::gfx::Matrix::Translate(5.0F, -3.0F),
			mocca::gfx::Matrix::Rotate(30.0F)
		);

		auto inverse = matrix.Invert();
		ASSERT_TRUE(inverse.has_value());

		mocca::Vector2 point{.X = 7.0F, .Y = 11.0F};
		auto back = inverse->MapPoint(matrix.MapPoint(point));

		EXPECT_NEAR(back.X, point.X, 0.0001F);
		EXPECT_NEAR(back.Y, point.Y, 0.0001F);
	}

	TEST(MatrixTest, MapRectBoundsRotation)
	{
		auto rotated = mocca::gfx::Matrix::Rotate(45.0F);
		auto bounds = rotated.MapRect(
			{.X = 0.0F, .Y = 0.0F, .Width = 10.0F, .Height = 10.0F}
		);

		EXPECT_NEAR(bounds.Width, 10.0F * std::sqrt(2.0F), 0.001F);
		EXPECT_NEAR(bounds.Height, 10.0F * std::sqrt(2.0F), 0.001F);
	}

	TEST(MatrixTest, TranslateOnlyClassification)
	{
		EXPECT_TRUE(mocca::gfx::Matrix::Identity().IsIdentity());
		EXPECT_TRUE(
			mocca::gfx::Matrix::Translate(3.0F, 4.0F).IsTranslateOnly()
		);
		EXPECT_FALSE(mocca::gfx::Matrix::Scale(2.0F, 2.0F).IsTranslateOnly());
	}

	TEST(GeometryTest, InsetRRectShrinks)
	{
		mocca::RRect outer{
			.Rect = {.X = 0, .Y = 0, .Width = 100, .Height = 50},
			.Radii = {
				.TopLeft = 10,
				.TopRight = 10,
				.BottomRight = 10,
				.BottomLeft = 10,
			},
		};
		mocca::EdgeSizes border{
			.Top = 5,
			.Right = 5,
			.Bottom = 5,
			.Left = 5,
		};

		auto inner = outer.Inset(border);

		EXPECT_FLOAT_EQ(inner.Rect.X, 5.0F);
		EXPECT_FLOAT_EQ(inner.Rect.Y, 5.0F);
		EXPECT_FLOAT_EQ(inner.Rect.Width, 90.0F);
		EXPECT_FLOAT_EQ(inner.Rect.Height, 40.0F);
		EXPECT_FLOAT_EQ(inner.Radii.TopLeft, 5.0F);
		EXPECT_FLOAT_EQ(inner.Radii.BottomRight, 5.0F);
	}

	TEST(GeometryTest, InsetRRectClampsRadii)
	{
		mocca::RRect outer{
			.Rect = {.X = 0, .Y = 0, .Width = 40, .Height = 40},
			.Radii = {
				.TopLeft = 4,
				.TopRight = 4,
				.BottomRight = 4,
				.BottomLeft = 4,
			},
		};
		mocca::EdgeSizes border{
			.Top = 8,
			.Right = 8,
			.Bottom = 8,
			.Left = 8,
		};

		auto inner = outer.Inset(border);

		EXPECT_FLOAT_EQ(inner.Radii.TopLeft, 0.0F);
		EXPECT_FLOAT_EQ(inner.Radii.TopRight, 0.0F);
		EXPECT_FLOAT_EQ(inner.Radii.BottomRight, 0.0F);
		EXPECT_FLOAT_EQ(inner.Radii.BottomLeft, 0.0F);
	}

	TEST(GeometryTest, EmptyRRectDetected)
	{
		EXPECT_TRUE(
			mocca::RRect(
				{.Rect = {.X = 0, .Y = 0, .Width = 0, .Height = 10},
				 .Radii = {}}
			).IsEmpty()
		);
		EXPECT_FALSE(
			mocca::RRect(
				{.Rect = {.X = 0, .Y = 0, .Width = 1, .Height = 10},
				 .Radii = {}}
			).IsEmpty()
		);
	}

	TEST(CanvasStateTest, SaveRestoreKeepsStreamBalanced)
	{
		mocca::Canvas canvas;
		canvas.Save();
		canvas.Translate(10.0F, 20.0F);
		canvas.Save();
		canvas.ClipRect({.X = 0, .Y = 0, .Width = 5, .Height = 5});
		canvas.Restore();
		canvas.Restore();

		auto cmds = canvas.Commands();
		EXPECT_EQ(saveCount(cmds), restoreCount(cmds));
		EXPECT_EQ(clipRectCount(cmds), 1);
		EXPECT_EQ(concatCount(cmds), 1);
	}

	TEST(CanvasStateTest, QuickRejectFollowsClipAndTransform)
	{
		mocca::Canvas canvas;
		canvas.ClipRect({.X = 0, .Y = 0, .Width = 10, .Height = 10});

		EXPECT_FALSE(
			canvas.QuickReject({.X = 0, .Y = 0, .Width = 5, .Height = 5})
		);

		canvas.Translate(100.0F, 0.0F);
		EXPECT_TRUE(
			canvas.QuickReject({.X = 0, .Y = 0, .Width = 5, .Height = 5})
		);
		EXPECT_FALSE(
			canvas.QuickReject({.X = -95.0F, .Y = 0, .Width = 5, .Height = 5})
		);
	}

	TEST(CanvasStateTest, QuickRejectConservativeOnRotation)
	{
		mocca::Canvas canvas;
		canvas.ClipRect({.X = 0, .Y = 0, .Width = 10, .Height = 10});
		canvas.Rotate(45.0F);

		EXPECT_FALSE(
			canvas.QuickReject({.X = 0, .Y = 0, .Width = 10, .Height = 10})
		);
		EXPECT_TRUE(
			canvas.QuickReject({.X = 100, .Y = 100, .Width = 10, .Height = 10})
		);
	}

	TEST(CanvasStateTest, AppendPreservesCommandStream)
	{
		mocca::Canvas child;
		child.Save();
		child.Translate(1.0F, 2.0F);
		child.Restore();

		mocca::Canvas parent;
		parent.Save();
		parent.Append(child);
		parent.Restore();

		auto cmds = parent.Commands();
		EXPECT_EQ(saveCount(cmds), restoreCount(cmds));
	}
}

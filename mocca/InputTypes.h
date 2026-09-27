#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace mocca
{
	enum class KeyCode : uint8_t
	{
		Unknown = 0,
		A = 4,
		B = 5,
		C = 6,
		D = 7,
		E = 8,
		F = 9,
		G = 10,
		H = 11,
		I = 12,
		J = 13,
		K = 14,
		L = 15,
		M = 16,
		N = 17,
		O = 18,
		P = 19,
		Q = 20,
		R = 21,
		S = 22,
		T = 23,
		U = 24,
		V = 25,
		W = 26,
		X = 27,
		Y = 28,
		Z = 29,
		Num0 = 30,
		Num1 = 31,
		Num2 = 32,
		Num3 = 33,
		Num4 = 34,
		Num5 = 35,
		Num6 = 36,
		Num7 = 37,
		Num8 = 38,
		Num9 = 39,
		Escape = 41,
		Enter = 43,
		Tab = 44,
		Backspace = 45,
		Space = 48,
		Left = 80,
		Right = 81,
		Up = 82,
		Down = 83,
		LShift = 225,
		RShift = 226,
		LCtrl = 227,
		RCtrl = 228,
		LAlt = 229,
		RAlt = 230,
	};

	struct PointerEvent
	{
	public:
		enum class Type : char
		{
			Down,
			Up,
			Move,
			Enter,
			Leave,
			Scroll
		};

		Type EventType = Type::Down;
		float X = 0.0F;
		float Y = 0.0F;
		int Button = 0;
		float ScrollX = 0.0F;
		float ScrollY = 0.0F;
		bool StopPropagation = false;
	};

	struct KeyEvent
	{
	public:
		enum class Type : char
		{
			Down,
			Repeat,
			Up
		};

		Type EventType = Type::Down;
		KeyCode Code = KeyCode::Unknown;
		bool StopPropagation = false;
	};

	struct TextEvent
	{
	public:
		enum class Type : char
		{
			Character,
			IMEStart, // handle these in ime-supporting backends like sdl or wayland
			IMEUpdate,
			IMEEnd
		};

		Type EventType = Type::Character;
		char32_t Codepoint = U'\0';
		std::string IMEComposition;
		bool StopPropagation = false;
	};

	struct SurfaceEvent
	{
	public:
		enum class Type : char
		{
			Resize,
			Close,
			FocusGained,
			FocusLost,
			DPIChange
		};

		Type EventType = Type::Resize;
		float Data1 = 0.0F;
		float Data2 = 0.0F;
	};

	struct InputBatch
	{
	public:
		std::vector<PointerEvent> Pointer;
		std::vector<KeyEvent> Keyboard;
		std::vector<TextEvent> Text;
		std::vector<SurfaceEvent> Surface;

		void Clear()
		{
			Pointer.clear();
			Keyboard.clear();
			Text.clear();
			Surface.clear();
		}
	};
}

#pragma once
#include "DrawCommands.h"
#include "InputTypes.h"
#include <vector>

namespace mocca
{
	class PlatformSurface
	{
	public:
		virtual ~PlatformSurface() = default;

		virtual void CollectEvents(InputBatch& batch) = 0;
		virtual bool ShouldClose() = 0;
		virtual void Submit(const std::vector<cmds::DrawCommand>& commands) = 0;
	};
}

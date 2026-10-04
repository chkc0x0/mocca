#include "core/Identifiers.h"

namespace mocca
{
	auto ApplicationID::ValidateID(std::string_view id) -> bool
	{
		if (id.empty())
		{
			return false;
		}

		int segments = 0;
		int segmentLen = 0;

		for (char c : id)
		{
			if (c == '.')
			{
				if (segmentLen == 0)
				{
					return false;
				}
				segments++;
				segmentLen = 0;
			}
			else if (
				(std::isalnum((unsigned char)c) != 0) || c == '_' || c == '-'
			)
			{
				segmentLen++;
			}
			else
			{
				return false;
			}
		}

		if (segmentLen == 0)
		{
			return false;
		}
		segments++;

		return segments == 2 || segments == 3;
	}
}
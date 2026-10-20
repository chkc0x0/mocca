#include "text/FontCoverage.h"
#include "core/Application.h"
#include "hb.h"
#include "text/FontRegistry.h"
#include <memory>
#include <unordered_map>

namespace mocca::mtext::detail
{
	auto FontCoverage::SetFor(FontFaceId face) -> const hb_set_t*
	{
		if (auto it = sets.find(face); it != sets.end())
		{
			return it->second.get();
		}

		auto set = SetPtr{hb_set_create(), hb_set_destroy};
		if (hb_face_t* hbFace = Application::main->Fonts().Face(face))
		{
			hb_face_collect_unicodes(hbFace, set.get());
		}

		return sets.emplace(face, std::move(set)).first->second.get();
	}

	auto FontCoverage::Covers(FontFaceId face, uint32_t codepoint) -> bool
	{
		const hb_set_t* set = SetFor(face);
		return (set != nullptr) && (hb_set_has(set, codepoint) != 0);
	}

	void FontCoverage::Clear()
	{
		sets.clear();
	}
}

#include "style/StyleValue.h"
#include "core/Application.h"
#include "utils/TypeInterners.h"

namespace mocca::styles::detail
{
	auto internTransition(const TransitionType& type) -> uint32_t
	{
		return Application::main->GetInterners()->Transitions.Intern(type);
	}

	auto resolveTransition(uint32_t ref) -> const TransitionType&
	{
		return Application::main->GetInterners()->Transitions.Get(ref);
	}

	auto internFontFamily(const FontFamilyType& family) -> uint32_t
	{
		return Application::main->GetInterners()->FontFamilies.Intern(family);
	}

	auto resolveFontFamily(uint32_t ref) -> const FontFamilyType&
	{
		return Application::main->GetInterners()->FontFamilies.Get(ref);
	}
}
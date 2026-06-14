#include "Application.h"
auto main(int argc, const char** argv) -> int
{
	auto app = mocca::Application("com.mocca.sandbox");
	app.SetDefaultLogCallback();

	app.Run();

	return 0;
}
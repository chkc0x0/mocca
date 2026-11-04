# mocca

a c++23 ui framework for reactive interfaces

## building

```sh
git clone https://github.com/chkc0x0/mocca --recursive
cd mocca
meson setup build
ninja -C build
```

## dependencies

- [yoga](https://github.com/react/yoga)
- [harfbuzz](https://github.com/harfbuzz/harfbuzz)
- [sheenbidi](https://github.com/Tehreer/SheenBidi)
- [stb_image](https://github.com/nothings/stb)
- [libunibreak](https://github.com/adah1972/libunibreak)
- [utf8proc](https://github.com/JuliaStrings/utf8proc)
- [simdutf](https://github.com/simdutf/simdutf)
- [glfw](https://github.com/glfw/glfw) (optional: `sandbox` option)
- [kurva](https://github.com/chkc0x0/kurva) (optional: `sandbox` option)
- [glew](https://github.com/nigels-com/glew) (optional: `sandbox` option)
- [gtest](https://github.com/google/googletest) (optional: `tests` option)
- [tracy](https://github.com/wolfpld/tracy) (optional: `tracy` option)

## getting started

first you gotta write your own [PlatformInterface](mocca/platform/PlatformInterface.h),
[PlatformSurface](mocca/platform/PlatformSurface.h), [RenderDevice](mocca/gfx/RenderDevice.h), and [Renderer](mocca/gfx/Renderer.h) implementations

then you gotta find and register a font (remind me to embed one)

(looking back at it it's a lot damn)

then you can write your app like normal, a bit like so:

```cpp
#include "MyPlatformInterface.h"
#include "core/Application.h"
#include "core/Element.h"
#include "core/Hooks.h"
#include "MyRenderDevice.h"
#include "widgets/Button.h"

using namespace mocca;

auto App() -> Element
{
 auto [counter, setCounter] = useState(0);

 return box({
  .Style =
   {
    mocca::styles::FontFamily("My Font"),
   },
  .Children = {
   widgets::basic::button({
    .Children =
     {
      text("Counter: " + std::to_string(counter)),
     },
    .OnPress = [setCounter](auto& ev) -> auto
    { setCounter([](auto& c) -> auto { return c + 1; }); },
   }),
  },
 });
}

auto main(int argc, const char** argv) -> int
{
 auto app = Application("com.my.app");
 app.SetRenderDevice<MyRenderDevice>();
 app.SetPlatformInterface(createMyPlatformInterface());

 app.Fonts().RegisterFile("../path/to/myFont.ttf", {.Family = "My Font"});

 app.RegisterSurface({
  .Width = 800,
  .Height = 600,
  .Title = "App",
  .Flags = SurfaceFlags::External,
  .Root = App,
 });

 while (app.IsRunning())
 {
  app.Tick();
  app.Render();
 }

 return 0;
}
```

if you're familiar with react it should be very familiar of course

## references

- [react](https://github.com/react/react) (duh)
- [flutter](https://github.com/flutter/flutter)
- [imgui](https://github.com/ocornut/imgui)
- [motion](https://github.com/motiondivision/motion)
- [skia](https://skia.googlesource.com/skia) (wink wink)
- [the world wide web](https://info.cern.ch/)
- [gpui](https://github.com/zed-industries/zed/tree/main/crates/gpui) (it's pretty good, along a few others. check them out!)
- react component libraries

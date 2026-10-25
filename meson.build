project('mocca', 'cpp', version: '0.0.1', default_options: ['cpp_std=c++23'])

sources = [
    'src/core/Application.cpp',
    'src/core/Logger.cpp',
    'src/core/Surface.cpp',
    'src/core/Node.cpp',
    'src/core/HookStore.cpp',
    'src/core/Context.cpp',
    'src/gfx/Canvas.cpp',
    'src/gfx/TextureRegistry.cpp',
    'src/gfx/TextureCache.cpp',
    'src/style/StyleApply.cpp',
    'src/core/Element.cpp',
    'src/core/HookFrame.cpp',
    'src/text/Measure.cpp',
    'src/text/Unicode.cpp',
    'src/core/Identifiers.cpp',
    'src/input/Scroll.cpp',
    'src/style/StyleList.cpp',
    'src/style/StyleSet.cpp',
    'src/style/ComputedStyle.cpp',
    'src/anim/Curve.cpp',
    'src/style/StyleValue.cpp',
    'src/text/FontRegistry.cpp',
    'src/text/FontMatcher.cpp',
    'src/text/HarfbuzzUtils.cpp',
    'src/text/TextResolve.cpp',
    'src/text/TextShaper.cpp',
    'src/text/Itemizer.cpp',
    'src/text/FontCoverage.cpp',
    'src/text/ShapeCache.cpp',
    'src/text/TextUtils.cpp',
    'src/text/TextLayoutBuilder.cpp',
    'src/text/LayoutCache.cpp',
    'src/text/Bidi.cpp'
]

cmake = import('cmake')

cmake_options = cmake.subproject_options()
cmake_options.add_cmake_defines({'WITH_PIC': true})

yoga_dep = cmake.subproject('yoga', options: cmake_options).dependency('yogacore')

dependencies = [
    yoga_dep, 
    dependency('freetype2'), 
    dependency('harfbuzz'), 
    subproject('SheenBidi').get_variable('sheenbidi_dep'),
    dependency('libunibreak'),
    dependency('libutf8proc'),
    dependency('simdutf')
]

if get_option('debug')
    add_project_arguments('-DDEBUG', language: 'cpp')
endif

if get_option('profile')
    add_project_arguments('-DMC_PROFILER', language: 'cpp')
    sources += ['src/utils/ScopedProfiler.cpp']
endif


mocca_lib = library(
    'mocca',
    sources: sources,
    dependencies: dependencies,
    include_directories: include_directories('mocca'),
    cpp_pch: 'mocca/core/pch.h',
)

mocca_dep = declare_dependency(
    link_with: mocca_lib,
    dependencies: dependencies,
    include_directories: include_directories('mocca'),
)

if get_option('sandbox')
    executable(
        'sandbox',
        dependencies: [mocca_dep, dependency('glfw3'), subproject('kurva').get_variable('nanovg_dep'), dependency('glew')],
        sources: [
            'sandbox/main.cpp',
            'sandbox/GLFWPlatformSurface.cpp',
            'sandbox/NVGRenderDevice.cpp',
            'sandbox/GLFWPlatformInterface.cpp'
        ],
        include_directories: include_directories('sandbox'),
    )
endif

if get_option('tests')
    test_exe = executable(
        'mocca_tests',
        dependencies: [mocca_dep, dependency('gtest', main: true)],
        sources: files(
            'test/AnimationTest.cpp',
            'test/ApplicationIdTest.cpp',
            'test/EventBusTest.cpp',
            'test/HookTest.cpp',
            'test/LayoutTest.cpp',
            'test/RenderTest.cpp',
            'test/SurfaceLifecycleTest.cpp',
            'test/FontTest.cpp',
            'test/TextLayoutTest.cpp'
        ),
        include_directories: include_directories('test'),
        cpp_args: [
            '-Dmc_testFont="@0@"'.format(
                meson.project_source_root() / 'sandbox' / 'Inter.ttf'
            ),
            '-Dmc_testCjkFont="@0@"'.format(
                meson.project_source_root() / 'sandbox' / 'NotoSansTC.ttf'
            )
        ],
        override_options: ['unity=on', 'unity_size=8'],
    )

    test('mocca', test_exe, timeout: 60)
endif

project('mocca', 'cpp', version: '0.0.1', default_options: ['cpp_std=c++23'])

sources = [
    'src/core/Application.cpp',
    'src/core/Logger.cpp',
    'src/core/Detail.cpp',
    'src/core/Surface.cpp',
    'src/core/Style.cpp',
]

cmake = import('cmake')

cmake_options = cmake.subproject_options()
cmake_options.add_cmake_defines({'WITH_PIC': true})

glfw_dep = dependency('', required: false)
kurva_dep = dependency('', required: false)

if get_option('sandbox')
    glfw_dep = cmake.subproject('glfw', options: cmake_options).dependency('glfw')
    kurva_dep = subproject('kurva').get_variable('nanovg_dep')
endif

yoga_dep = cmake.subproject('yoga', options: cmake_options).dependency('yogacore')

dependencies = [yoga_dep]
if get_option('sandbox')
    dependencies += [glfw_dep]
endif

if get_option('debug')
    add_project_arguments('-DDEBUG', language: 'cpp')
endif

mocca_lib = library(
    'mocca',
    sources: sources,
    dependencies: dependencies,
    include_directories: include_directories('mocca'),
)

mocca_dep = declare_dependency(
    link_with: mocca_lib,
    dependencies: dependencies,
    include_directories: include_directories('mocca'),
)

if get_option('sandbox')
    executable(
        'sandbox',
        dependencies: [mocca_dep, glfw_dep, kurva_dep, dependency('glew')],
        sources: [
            'sandbox/main.cpp',
            'sandbox/GLFWPlatformSurface.cpp',
        ],
        include_directories: include_directories('sandbox'),
    )
endif

if get_option('tests')
    test_exe = executable(
        'mocca_tests',
        dependencies: [mocca_dep, dependency('gtest', main: true)],
        sources: files(
            'test/RenderTest.cpp',
            'test/HookTest.cpp',
            'test/EventBusTest.cpp',
            'test/SurfaceLifecycleTest.cpp',
            'test/ApplicationIdTest.cpp',
        ),
        include_directories: include_directories('test'),
    )

    test('mocca', test_exe, timeout: 60)
endif

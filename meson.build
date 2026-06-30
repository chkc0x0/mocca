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

glfw_dep = cmake.subproject('glfw', options: cmake_options).dependency('glfw')
yoga_dep = cmake.subproject('yoga', options: cmake_options).dependency('yogacore')
nanovg_dep = subproject('nanovg').get_variable('nanovg_dep')

dependencies = [glfw_dep, yoga_dep]

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

sandbox_inc = include_directories('sandbox', 'sandbox/glad/include')

executable(
    'sandbox',
    dependencies: [mocca_dep, glfw_dep, nanovg_dep],
    sources: [
        'sandbox/main.cpp',
        'sandbox/GLFWPlatformSurface.cpp',
    ],
    include_directories: sandbox_inc,
)

project('mocca', 'cpp', version: '0.0.1', default_options: ['cpp_std=c++23'])

sources = ['src/core/Application.cpp', 'src/core/Logger.cpp', 'src/core/Detail.cpp']

cmake = import('cmake')

cmake_options = cmake.subproject_options()
cmake_options.add_cmake_defines({'WITH_PIC': true})

dependencies = [cmake.subproject('raylib', options: cmake_options).dependency('raylib')]

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

executable('sandbox', dependencies: mocca_dep, sources: ['sandbox/main.cpp'])

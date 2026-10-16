project('simdutf', 'cpp')

sources = ['simdutf.cpp']

simdutf_lib = library('simdutf', sources: sources)
simdutf_dep = declare_dependency(link_with: simdutf_lib, include_directories: include_directories('.'))

meson.override_dependency('simdutf', simdutf_dep)
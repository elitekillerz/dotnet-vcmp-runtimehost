project('elitekillerz-dotnetvcmp-runtimehost', 'cpp', 'c', default_options: ['cpp_std=c++17'])

inc_dirs = include_directories('vendor')

cc = meson.get_compiler('c')
source_dir = meson.current_source_dir()
dist_dir = join_paths(source_dir, 'dist')
lib_nethost = cc.find_library('nethost', dirs: [dist_dir])

shared_module('dotnetvcmp',
  'src/main.cxx',
  include_directories: inc_dirs,
  dependencies: [lib_nethost],
  build_rpath: '.',
  name_prefix: 'mod_')

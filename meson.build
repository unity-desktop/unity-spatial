project(
  'unity-spatial',
  ['c', 'cpp'],
  version: '26',
  license: 'GPL-3.0-or-later',
  meson_version: '>= 1.0',
  default_options: [
    'cpp_std=c++17',
    'warning_level=2',
  ],
)

dep_wayfire  = dependency('wayfire')
dep_wfconfig = dependency('wf-config')
dep_wlroots  = dependency('wlroots-0.20')
dep_wlserver = dependency('wayland-server')

wayland_scanner = find_program('wayland-scanner')

add_project_arguments(
  '-I' + dep_wayfire.get_variable(pkgconfig: 'includedir') / 'wayfire',
  language: 'cpp',
)

plugindir   = get_option('libdir') / 'wayfire'
metadatadir = get_option('datadir') / 'wayfire' / 'metadata'

subdir('wayfire-plugins' / 'unity-preview')

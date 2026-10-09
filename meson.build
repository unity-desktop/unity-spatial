project(
  'unity-spatial',
  ['c', 'cpp'],
  version: '26',
  license: 'GPL-3.0-or-later',
  meson_version: '>= 1.0',
  default_options: [
    'c_std=gnu11',
    'cpp_std=c++17',
    'warning_level=2',
  ],
)

dep_gtk         = dependency('gtk4', version: '>= 4.22')
dep_gtk_wayland = dependency('gtk4-wayland')
dep_astalwl     = dependency('astal-wl-0.1')
dep_astalapps   = dependency('astal-apps-0.1')
dep_wlclient    = dependency('wayland-client')
dep_gio_unix    = dependency('gio-unix-2.0')
dep_json        = dependency('json-glib-1.0')
dep_libdex      = dependency('libdex-1')
dep_desktop_schemas = dependency('gsettings-desktop-schemas')
dep_unity_platform_backgrounds = dependency('unity-platform-backgrounds')
dep_unity_platform_components  = dependency('unity-platform-components')
dep_wlserver    = dependency('wayland-server')
dep_wayfire     = dependency('wayfire')
dep_wfconfig    = dependency('wf-config')
dep_wlroots     = dependency('wlroots-0.20')
dep_wlprotocols = dependency('wayland-protocols')

cc = meson.get_compiler('c')

add_project_arguments(
  cc.get_supported_arguments(['-Wno-unused-parameter']),
  '-DG_LOG_DOMAIN="unity-spatial"',
  language: 'c',
)

add_project_arguments(
  '-I' + dep_wayfire.get_variable(pkgconfig: 'includedir') / 'wayfire',
  language: 'cpp',
)

gnome           = import('gnome')
wayland_scanner = find_program('wayland-scanner')
preview_xml     = files('wayfire-plugins' / 'unity-spatial-preview' / 'unity-spatial-preview-unstable.xml')
gestures_xml    = files('wayfire-plugins' / 'unity-spatial-gestures' / 'unity-spatial-gestures-unstable.xml')
protocols_dir   = dep_wlprotocols.get_variable(pkgconfig: 'pkgdatadir')

plugindir   = get_option('libdir') / 'wayfire'
metadatadir = get_option('datadir') / 'wayfire' / 'metadata'

subdir('data')
subdir('src')
subdir('wayfire-plugins' / 'unity-spatial-preview')
subdir('wayfire-plugins' / 'unity-spatial-gestures')

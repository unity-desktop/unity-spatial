unity_spatial_deps = [
  dep_gtk,
  dep_gtk_wayland,
  dep_astalwl,
  dep_astalapps,
  dep_wlclient,
  dep_gio_unix,
  dep_json,
  dep_libdex,
  dep_unity_platform_backgrounds,
  dep_unity_platform_components,
]

unity_spatial_sources         = []
unity_spatial_headers         = []
unity_spatial_private_sources = []
unity_spatial_generated       = []

unity_spatial_includes = include_directories('.', 'carousel', 'gesture', 'window', 'workspace', 'desktop', 'models/window', 'models/workspace', 'spatialview', 'protocols', 'wayfire-ipc')

subdir('protocols')
subdir('wayfire-ipc')
subdir('models')
subdir('desktop')
subdir('gesture')
subdir('window')
subdir('workspace')
subdir('carousel')
subdir('spatialview')


unity_spatial_enums = gnome.mkenums_simple(
  'unity-spatial-enums',
  sources: files('desktop/unity-spatial-desktop.h', 'spatialview/unity-spatial-view.h'),
  install_header: true,
  install_dir: get_option('includedir') / 'unity-spatial',
)

unity_spatial_generated += unity_spatial_enums

unity_spatial_generated += gnome.compile_resources(
  'unity-spatial-resources',
  'unity-spatial.gresource.xml',
  c_name: 'unity_spatial',
)

unity_spatial_lib = library(
  'unity-spatial',
  unity_spatial_sources,
  unity_spatial_private_sources,
  unity_spatial_generated,
  dependencies: unity_spatial_deps,
  include_directories: unity_spatial_includes,
  version: meson.project_version(),
  soversion: '0',
  install: true,
)

install_headers(
  unity_spatial_headers,
  subdir: 'unity-spatial',
)

unity_spatial_dep = declare_dependency(
  link_with: unity_spatial_lib,
  include_directories: unity_spatial_includes,
  dependencies: unity_spatial_deps,
)

pkg = import('pkgconfig')
pkg.generate(
  unity_spatial_lib,
  name: 'unity-spatial',
  description: 'Unity spatial view',
  subdirs: 'unity-spatial',
  requires: [dep_gtk],
)

gnome.generate_gir(
  unity_spatial_lib,
  sources: unity_spatial_sources + unity_spatial_headers + unity_spatial_enums,
  namespace: 'UnitySpatial',
  nsversion: '0',
  identifier_prefix: 'UnitySpatial',
  symbol_prefix: 'unity_spatial',
  includes: ['Gtk-4.0'],
  dependencies: unity_spatial_deps,
  header: 'unity-spatial/unity-spatial-desktop.h',
  install: true,
)

executable(
  'unity-spatial',
  'unity-spatial-main.c',
  dependencies: [unity_spatial_dep, dep_desktop_schemas],
  link_args: ['-Wl,--no-as-needed', '-lgtk4-layer-shell', '-Wl,--as-needed'],
  install: true,
)

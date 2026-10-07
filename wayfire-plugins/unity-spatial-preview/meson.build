preview_protocol = [
  custom_target(
    input: preview_xml,
    output: '@BASENAME@-protocol.h',
    command: [wayland_scanner, 'server-header', '@INPUT@', '@OUTPUT@'],
  ),
  custom_target(
    input: preview_xml,
    output: '@BASENAME@-protocol.c',
    command: [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@'],
  ),
]

shared_module(
  'unity-spatial-preview',
  'plugin.cpp',
  'preview.cpp',
  'preview-node.cpp',
  'protocol.cpp',
  'stacking.cpp',
  preview_protocol,
  name_prefix: 'lib',
  dependencies: [dep_wayfire, dep_wfconfig, dep_wlroots, dep_wlserver],
  install: true,
  install_dir: plugindir,
)

install_data('unity-spatial-preview.xml', install_dir: metadatadir)

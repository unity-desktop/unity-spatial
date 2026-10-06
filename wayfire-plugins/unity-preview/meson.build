unity_preview_protocol = [
  custom_target(
    'unity-preview-server-header',
    input: 'unity-preview-v1.xml',
    output: '@BASENAME@-protocol.h',
    command: [wayland_scanner, 'server-header', '@INPUT@', '@OUTPUT@'],
  ),
  custom_target(
    'unity-preview-server-code',
    input: 'unity-preview-v1.xml',
    output: '@BASENAME@-protocol.c',
    command: [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@'],
  ),
]

shared_module(
  'unity-preview',
  'plugin.cpp',
  'mirror.cpp',
  'mirror-node.cpp',
  unity_preview_protocol,
  name_prefix: 'lib',
  dependencies: [dep_wayfire, dep_wfconfig, dep_wlroots, dep_wlserver],
  install: true,
  install_dir: plugindir,
)

install_data('unity-preview.xml', install_dir: metadatadir)
install_data('unity-preview-v1.xml', install_dir: get_option('datadir') / 'unity-spatial')

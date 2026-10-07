gestures_protocol = [
  custom_target(
    input: gestures_xml,
    output: '@BASENAME@-protocol.h',
    command: [wayland_scanner, 'server-header', '@INPUT@', '@OUTPUT@'],
  ),
  custom_target(
    input: gestures_xml,
    output: '@BASENAME@-protocol.c',
    command: [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@'],
  ),
]

shared_module(
  'unity-spatial-gestures',
  'plugin.cpp',
  'capture.cpp',
  'protocol.cpp',
  gestures_protocol,
  name_prefix: 'lib',
  dependencies: [dep_wayfire, dep_wfconfig, dep_wlroots, dep_wlserver],
  install: true,
  install_dir: plugindir,
)

install_data('unity-spatial-gestures.xml', install_dir: metadatadir)

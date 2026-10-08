unity_spatial_sources += files(
  'unity-spatial-preview-mirror.c',
)

unity_spatial_headers += files(
  'unity-spatial-preview-mirror.h',
)

unity_spatial_private_sources += files(
  'unity-spatial-gesture.c',
  'unity-spatial-globals.c',
  'unity-spatial-mirror-stack.c',
)

foreach xml : [preview_xml,
               gestures_xml,
               protocols_dir / 'stable' / 'viewporter' / 'viewporter.xml',
               protocols_dir / 'staging' / 'single-pixel-buffer' / 'single-pixel-buffer-v1.xml']
  unity_spatial_generated += custom_target(
    input: xml,
    output: '@BASENAME@-client-protocol.h',
    command: [wayland_scanner, 'client-header', '@INPUT@', '@OUTPUT@'],
  )
  unity_spatial_generated += custom_target(
    input: xml,
    output: '@BASENAME@-protocol.c',
    command: [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@'],
  )
endforeach

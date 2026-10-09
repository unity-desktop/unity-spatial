install_data(
  'org.unity.spatial.gschema.xml',
  install_dir: get_option('datadir') / 'glib-2.0' / 'schemas',
)

gnome.compile_schemas(depend_files: files('org.unity.spatial.gschema.xml'))
gnome.post_install(glib_compile_schemas: true)

conf = configuration_data()
conf.set('UNITY_SPATIAL_BIN', get_option('prefix') / get_option('bindir') / 'unity-spatial')

configure_file(
  input: 'unity-spatial.service.in',
  output: 'unity-spatial.service',
  configuration: conf,
  install: true,
  install_dir: get_option('datadir') / 'systemd' / 'user',
)

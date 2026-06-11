procvue_edj = custom_target('procvue.edj',
  input        : 'procvue.edc',
  output       : 'procvue.edj',
  command      : [
    edje_cc,
    '-id', meson.current_source_dir(),
    '-fd', meson.current_source_dir(),
    '@INPUT@', '@OUTPUT@',
  ],
  build_by_default : true,
  install          : true,
  install_dir      : get_option('prefix') / get_option('datadir') / 'procvue',
)

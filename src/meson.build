executable('procvue',
  sources      : files(
    'main.c',
    'enigmatic.c',
    'poll.c',
    'disk.c',
    'uptime.c',
    'config.c',
  ),
  dependencies : deps,
  c_args       : ['-DPROCVUE_DATADIR="' + (get_option('prefix') / get_option('datadir') / 'procvue') + '"'],
  install      : true,
  install_dir  : get_option('bindir'),
)

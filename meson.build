# meson.build
project('procvue', 'c',
  version : '0.1.0',
  default_options : [
    'c_std=gnu99',
    'warning_level=2',
  ],
)

cc = meson.get_compiler('c')

# ── pkg-config dependencies ────────────────────────────────────────────────
ecore      = dependency('ecore')
ecore_evas = dependency('ecore-evas')
edje_dep   = dependency('edje')
eio        = dependency('eio')
eina       = dependency('eina')
enigmatic  = dependency('enigmatic_client')
eet        = dependency('eet')

m_dep = cc.find_library('m', required : true)
all_deps = [ecore, ecore_evas, edje_dep, eio, eina, enigmatic, eet, m_dep]

# ── EDJ compilation ───────────────────────────────────────────────────────
edje_cc = find_program('edje_cc', required : true)

procvue_edj = custom_target('procvue.edj',
  input        : 'data/procvue.edc',
  output       : 'procvue.edj',
  command      : [
    edje_cc,
    '-id', meson.current_source_dir() / 'data',
    '-fd', meson.current_source_dir() / 'data',
    '@INPUT@', '@OUTPUT@',
  ],
  build_by_default : true,
  install          : true,
  install_dir : get_option('prefix') / get_option('datadir') / 'procvue',
)

# ── Executable ────────────────────────────────────────────────────────────
src = files(
  'src/main.c',
  'src/enigmatic.c',
  'src/poll.c',
  'src/disk.c',
  'src/uptime.c',
  'src/config.c',
)

procvue_exe = executable('procvue',
  sources      : src,
  dependencies : all_deps,
  c_args       : ['-DPROCVUE_DATADIR="' + (get_option('prefix') / get_option('datadir') / 'procvue') + '"'],
  install      : true,
  install_dir  : get_option('bindir'),
)

# ── Data install ──────────────────────────────────────────────────────────
share_dir = get_option('datadir') / 'procvue'  # $prefix/share/procvue

# ── Desktop integration ───────────────────────────────────────────────────
install_data('data/tacho-128.png',
  install_dir : get_option('datadir') / 'icons' / 'hicolor' / '128x128' / 'apps',
  rename      : 'procvue.png',
)

install_data('procvue.desktop',
  install_dir : get_option('datadir') / 'applications',
)
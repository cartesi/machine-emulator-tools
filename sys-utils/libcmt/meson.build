project('libcmt', 'c',
  default_options: ['default_library=both'],
  version : run_command('tools/version.sh', 'get-vcs').stdout().strip())
meson.add_dist_script('tools/version.sh', 'set-dist', meson.project_version())

build = meson.get_compiler('c', native: true)
host  = meson.get_compiler('c', native: false)

inccmt = include_directories('src')
install_headers(
  'src/abi.h',
  'src/buf.h',
  'src/io.h',
  'src/keccak.h',
  'src/merkle.h',
  'src/rollup.h',
  'src/util.h')

src_base = files(
  'src/abi.c',
  'src/buf.c',
  'src/keccak.c',
  'src/merkle.c',
  'src/rollup.c',
  'src/util.c')

libcmt = library('cmt',
  src_base + 'src/io.c',
  override_options: ['b_coverage=false'],
  install: true,
  native: false)

pkg = import('pkgconfig')
pkg.generate(libraries: libcmt,
  version: run_command('tools/version.sh', 'get-vcs').stdout().strip(),
  name: 'libcmt',
  filebase: 'libcmt',
  description: 'libcmt')

libmock = library('cmt-mock',
  src_base + 'src/io-mock.c',
  native: true)

tests = [
  'abi-multi',
  'abi-single',
  'buf',
  'io',
  'gio',
  'keccak',
  'merkle',
  'progress',
  'rollup',
  ]

foreach name: tests
  src = 'tests/' + name + '.c'
  exe = executable(name, src, include_directories: inccmt, link_with: libmock, native: true)
  test(name, exe)
endforeach

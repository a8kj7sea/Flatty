project('flatty', 'c',
  version : '0.1.0',
  default_options : ['c_std=c11', 'warning_level=3'])

add_project_arguments('-D_GNU_SOURCE', language: 'c')

inc = include_directories('include')

flatty_sources = [
  'src/main.c',
  'src/core/context.c',
  'src/core/config_loader.c',
  'src/core/walker.c',
  'src/core/writer.c',
  'src/ignore/ignore_parser.c',
  'src/utils/string_utils.c'
]

conf_data = configuration_data()
conf_data.set_quoted('FLATTY_DEFAULTS_DIR', join_paths(get_option('prefix'), get_option('datadir'), 'flatty'))
config_h = configure_file(
  output: 'flatty_config.h',
  configuration: conf_data
)

flatty_exe = executable('flatty',
  flatty_sources,
  include_directories : [inc, include_directories('.')],
  install : true)

install_data('defaults/config.conf', 'defaults/ignore.conf', install_dir : join_paths(get_option('datadir'), 'flatty'))

test('CLI Tests', find_program('tests/test_cli.sh'), args : flatty_exe)
test('Ignore Rules Test', executable('test_ignore', 
  'tests/test_ignore.c', 
  'src/ignore/ignore_parser.c',
  'src/utils/string_utils.c',
  include_directories : inc))
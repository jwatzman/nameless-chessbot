project(
	'nameless-chessbot',
	'c',
	default_options: [
		'buildtype=release',
		'b_lto=true',
		'debug=true',
		'warning_level=3',
	]
)

project_args = []

if meson.get_compiler('c').get_id() == 'clang' and get_option('warning_level').to_int() > 1
	project_args += [
		'-Wimplicit-int-conversion',
		'-Wsign-conversion',
		#'-Weverything',
	]
endif

cpu = target_machine.cpu_family()
if cpu == 'x86_64'
	project_args += ['-march=native', '-mtune=native']
elif cpu == 'aarch64'
	project_args += ['-mcpu=native']
endif

add_project_arguments(project_args, language: 'c')

incl_src = include_directories('src')

subdir('gen')
subdir('src')

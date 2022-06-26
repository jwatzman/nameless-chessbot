project(
	'nameless-chessbot',
	'c',
	default_options: [
		'buildtype=release',
		'b_lto=true',
		'c_args=-march=native -mtune=native',
		'debug=true',
		'warning_level=3',
	]
)

incl_src = include_directories('src')

subdir('gen')
subdir('src')

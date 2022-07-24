libcore = static_library(
	'core',
	['bitboard.c', 'move.c', 'movemagic.c', 'nnue.c', nnue_bin_c, move_h],
)

libsearch = static_library(
	'search',
	['evaluate.c', 'history.c', 'moveiter.c', 'search.c', 'statelist.c', 'timer.c', 'tt.c', evaluate_h],
)

libperft = static_library(
	'perft',
	['perftfn.c'],
)

executable(
	'perft',
	'perft.c',
	link_with: [libcore, libperft],
)

test(
	'perft',
	executable(
		'test-perft',
		'test-perft.c',
		link_with: [libcore, libperft],
	),
	protocol: 'tap',
	timeout: 120,
)

test(
	'search',
	executable(
		'test-search',
		'test-search.c',
		link_with: [libcore, libsearch],
	),
	protocol: 'tap',
	timeout: 120,
)

executable(
	'search-perf',
	'search-perf.c',
	link_with: [libcore, libsearch],
)

executable(
	'console',
	'console.c',
	link_with: [libcore, libsearch],
)

executable(
	'xboard',
	'xboard.c',
	link_with: [libcore, libsearch, libperft],
)

executable(
	'nnue-training-data',
	'nnue-training-data.c',
	link_with: [libcore, libsearch],
)

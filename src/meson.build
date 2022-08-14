libcore = static_library(
	'core',
	['bitboard.c', 'move.c', 'movemagic.c', 'mt19937ar.c', 'nnue.c', move_h],
	link_depends: [nnue_bin],
)

libsearch = static_library(
	'search',
	['evaluate.c', 'history.c', 'moveiter.c', 'search.c', 'see.c', 'statelist.c', 'timer.c', 'tt.c', evaluate_h],
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
)

test(
	'search',
	executable(
		'test-search',
		'test-search.c',
		link_with: [libcore, libsearch],
	),
	protocol: 'tap',
)

test(
	'see',
	executable(
		'test-see',
		'test-see.c',
		link_with: [libcore, libsearch],
	),
	protocol: 'tap',
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
	'nameless-xboard',
	'xboard.c',
	link_with: [libcore, libsearch, libperft],
	install: true,
)

executable(
	'nnue-training-data',
	'nnue-training-data.c',
	link_with: [libcore, libsearch],
)

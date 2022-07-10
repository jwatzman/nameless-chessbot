core = ['bitboard.c', 'move.c', 'moveiter.c', 'movemagic.c', move_h]
all = core + ['evaluate.c', 'history.c', 'search.c', 'statelist.c', 'timer.c', 'tt.c', evaluate_h]

executable(
	'perft',
	core + 'perftfn.c' + 'perft.c',
)

test(
	'perft',
	executable(
		'test-perft',
		core + 'perftfn.c' + 'test-perft.c',
	),
	protocol: 'tap',
	timeout: 120,
)

test(
	'search',
	executable(
		'test-search',
		all + 'test-search.c',
	),
	protocol: 'tap',
	timeout: 120,
)

executable(
	'search-perf',
	all + 'search-perf.c',
)

executable(
	'console',
	all + 'console.c',
)

executable(
	'xboard',
	all + 'perftfn.c' + 'xboard.c',
)

core = ['bitboard.c', 'move.c', 'moveiter.c', 'movemagic.c', move_h]
all = core + ['evaluate.c', 'search.c', 'timer.c', evaluate_h]

executable(
	'perft',
	core + 'perftfn.c' + 'perft.c',
)

executable(
	'test-perft',
	core + 'perftfn.c' + 'test-perft.c',
)

executable(
	'stress',
	core + 'stress.c',
)

executable(
	'test-search',
	all + 'test-search.c',
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
	all + 'xboard.c',
)

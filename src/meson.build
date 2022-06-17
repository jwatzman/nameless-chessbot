core = ['bitboard.c', 'move.c', 'moveiter.c', 'movemagic.c', move_h]
all = core + ['evaluate.c', 'search.c', 'timer.c', evaluate_h]

executable(
	'perft',
	core + 'perft.c',
)

executable(
	'stress',
	core + 'stress.c',
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
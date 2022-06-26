gen_move = executable(
	'gen_move',
	['move_king.c', 'move_knight.c', 'move_pawn.c', 'move_raycast.c', 'move.c'],
	include_directories: incl_src,
)

gen_eval = executable(
	'gen_eval',
	['eval_front_spans.c', 'eval.c'],
	include_directories: incl_src,
)

move_h = custom_target(
	output: 'move.h',
	command: gen_move,
	capture: true,
)

evaluate_h = custom_target(
	output: 'evaluate.h',
	command: gen_eval,
	capture: true,
)

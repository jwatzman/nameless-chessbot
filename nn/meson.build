nnue_bin_local = custom_target(
	input: 'nnue.bin',
	output: 'nnue.bin',
	command: ['cp', '@INPUT@', '@OUTPUT@'],
)

nnue_bin_c = custom_target(
	input: nnue_bin_local,
	output: 'nnue.bin.c',
	command: ['xxd', '-i', '@INPUT@'],
	capture: true,
)

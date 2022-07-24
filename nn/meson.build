nnue_bin = custom_target(
	input: 'nnue.bin.zst',
	output: 'nnue.bin',
	command: ['unzstd', '-f', '@INPUT@', '-o', '@OUTPUT@'],
)

nnue_bin_c = custom_target(
	input: nnue_bin,
	output: 'nnue.bin.c',
	command: ['xxd', '-i', '@INPUT@'],
	capture: true,
)

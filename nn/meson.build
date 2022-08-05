nnue_bin = custom_target(
	input: 'nnue.bin.zst',
	output: 'nnue.bin',
	command: ['unzstd', '-f', '@INPUT@', '-o', '@OUTPUT@'],
)

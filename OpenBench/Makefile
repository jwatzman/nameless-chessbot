xboard:
ifneq ($(EVALFILE),)
	cp $(EVALFILE) ../nn/nnue.bin.zst
endif
	CC=$(CC) meson setup -Db_ndebug=true -Dc_args="-DRANDOM_FIRST_MOVE=1" ../ build
	ninja -v -C build src/xboard
	cp build/src/xboard $(EXE)

.PHONY: xboard

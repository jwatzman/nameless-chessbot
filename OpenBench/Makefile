xboard:
ifneq ($(EVALFILE),)
	cp $(EVALFILE) ../nn/nnue.bin.zst
endif
	CC=$(CC) meson setup -Db_ndebug=true ../ build
	ninja -C build src/nameless-xboard
	cp build/src/nameless-xboard $(EXE)

.PHONY: xboard

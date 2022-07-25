xboard:
	CC=$(CC) meson setup -Db_ndebug=true ../ build
	ninja -C build src/xboard
	cp build/src/xboard $(EXE)

.PHONY: xboard

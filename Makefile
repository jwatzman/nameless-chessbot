CFLAGS=-O3 --std=c99 -Wall -Wextra
SOURCES_CORE=bitboard.c move.c movelist.c
SOURCES=${SOURCES_CORE} evaluate.c search.c

all: xboard

perft:
	gcc ${CFLAGS} -o perft ${SOURCES_CORE} perft.c

test:
	gcc ${CFLAGS} -o test ${SOURCES} test.c

xboard:
	gcc ${CFLAGS} -o ncb-xboard ${SOURCES} xboard.c

gen:
	./move-generated-generator.sh

clean:
	-rm -f perft test ncb-xboard move-generated.h

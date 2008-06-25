CFLAGS=-O3 --std=c99 -Wall -Wextra
SOURCES_CORE=bitboard.c move.c movelist.c
SOURCES=${SOURCES_CORE} evaluate.c search.c

all: perft test xboard

perft: ${SOURCES_CORE} perft.c
	gcc ${CFLAGS} -o perft ${SOURCES_CORE} perft.c

test: ${SOURCES} test.c
	gcc ${CFLAGS} -o test ${SOURCES} test.c

xboard: ${SOURCES} xboard.c
	gcc ${CFLAGS} -o ncb-xboard ${SOURCES} xboard.c

gen:
	./move-generated-generator.sh

clean:
	-rm -f perft test ncb-xboard move-generated.h

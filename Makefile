CFLAGS=-O3 --std=c99 -Wall -Wextra
SOURCES_GEN=bb-tablegen-col.c bb-tablegen-diag315.c bb-tablegen-diag45.c bb-tablegen-king.c bb-tablegen-knight.c bb-tablegen-pawn.c bb-tablegen-row.c
SOURCES_CORE=bitboard.c move.c
SOURCES=${SOURCES_CORE} evaluate.c search.c

all: perft test xboard

perft: ${SOURCES_CORE} perft.c move-generated.h
	gcc ${CFLAGS} -o perft ${SOURCES_CORE} perft.c

test: ${SOURCES} test.c move-generated.h
	gcc ${CFLAGS} -o test ${SOURCES} test.c

xboard: ${SOURCES} xboard.c move-generated.h
	gcc ${CFLAGS} -o xboard ${SOURCES} xboard.c

move-generated.h: ${SOURCES_GEN}
	./move-generated-generator.sh

clean:
	-rm -f perft test ncb-xboard move-generated.h

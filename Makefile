# basic CFLAGS
CFLAGS=-O3 --std=c99 -Wall -Wextra -march=core2

# STFU about printf arguments
CFLAGS+=-Wno-format

# we're compiling all at once
CFLAGS+=-fwhole-program -combine

# this helps perft
CFLAGS+=-funroll-loops

# more flags that are not marked as unsafe but are not enabled by default
# (these don't *hurt* perft, but it's unclear if they actually help)
#CFLAGS+=-fipa-struct-reorg -fipa-cp -fgcse-sm -fgcse-las -ftracer -funswitch-loops -fbranch-target-load-optimize -fbranch-target-load-optimize2

# yeah yeah yeah, but this causes no warnings...
CFLAGS+=-funsafe-loop-optimizations -Wunsafe-loop-optimizations

SOURCES_CORE=bitboard.c move.c
SOURCES=${SOURCES_CORE} evaluate.c search.c

all: perft test xboard

perft: ${SOURCES_CORE} perft.c move-generated.h
	gcc ${CFLAGS} -o perft ${SOURCES_CORE} perft.c

test: ${SOURCES} test.c move-generated.h evaluate-generated.h
	gcc ${CFLAGS} -o test ${SOURCES} test.c

xboard: ${SOURCES} xboard.c move-generated.h evaluate-generated.h
	gcc ${CFLAGS} -o xboard ${SOURCES} xboard.c

move-generated.h:
	generators/move/generate.sh > move-generated.h

evaluate-generated.h:
	generators/evaluate/generate.sh > evaluate-generated.h

clean:
	-rm -f perft test xboard move-generated.h evaluate-generated.h

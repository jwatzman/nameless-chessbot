# basic CFLAGS
# CFLAGS=-O3 --std=c99 -Wall -Wextra -march=core2 -pg

# STFU about printf arguments
# CFLAGS+=-Wno-format

# we're compiling all at once
# CFLAGS+=-fwhole-program -combine

# this helps perft
# CFLAGS+=-funroll-loops

# yeah yeah yeah, but this causes no warnings...
# CFLAGS+=-funsafe-loop-optimizations -Wunsafe-loop-optimizations

# please please forgive me for this; they're from asubrama
# they drop "perft 6" from ~25 seconds to ~15 seconds with the flags above
CFLAGS=-Wall -Wextra -Wformat=2 -Wstrict-aliasing=1 -Wcast-qual -Wcast-align
CFLAGS+=-Wunsafe-loop-optimizations -Wfloat-equal -Waggregate-return -Wswitch-default -Wmissing-prototypes
CFLAGS+=-Wno-format
CFLAGS+=-O3 -fgcse-las -fgcse-sm -fsee -ftree-loop-linear -fivopts -fweb -fomit-frame-pointer
CFLAGS+=-march=core2 -mfpmath=sse,387
CFLAGS+=-fwhole-program -fipa-matrix-reorg -fipa-struct-reorg -combine
CFLAGS+=-funroll-loops -funsafe-loop-optimizations --std=c99

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

ifeq ($(origin CC), default)
	export CC=clang
endif

export LDFLAGS=-O3 -Wall -Wextra
export CFLAGS=$(LDFLAGS) -emit-llvm -std=c99
#export CFLAGS+=-O0 -ftrapv -fcatch-undefined-behavior -g

SOURCES_CORE=bitboard.o move.o moveiter.o
SOURCES=$(SOURCES_CORE) evaluate.o search.o timer.o

all: perft stress search-perf test xboard

perft: $(SOURCES_CORE) perft.o
	$(CC) $(LDFLAGS) -o perft $(SOURCES_CORE) perft.o

stress: $(SOURCES_CORE) stress.o
	$(CC) $(LDFLAGS) -o stress $(SOURCES_CORE) stress.o

search-perf: $(SOURCES) search-perf.o
	$(CC) $(LDFLAGS) -o search-perf $(SOURCES) search-perf.o

test: $(SOURCES) test.o
	$(CC) $(LDFLAGS) -o test $(SOURCES) test.o

xboard: $(SOURCES) xboard.o
	$(CC) $(LDFLAGS) -o xboard $(SOURCES) xboard.o

move-generated.h:
	generators/move/generate.sh > move-generated.h

evaluate-generated.h:
	generators/evaluate/generate.sh > evaluate-generated.h

headers: move-generated.h evaluate-generated.h
move.o: move-generated.h
evaluate.o: evaluate-generated.h

%.o: %.c
	$(CC) $(CFLAGS) -c $*.c -o $*.o

clean:
	-rm -f perft stress test xboard move-generated.h evaluate-generated.h *.o

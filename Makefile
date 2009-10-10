ifeq ($(origin CC), default)
	export CC=clang
endif

export CFLAGS=-O3 -std=c99 -Wall -Wextra

SOURCES_CORE=bitboard.o move.o
SOURCES=$(SOURCES_CORE) evaluate.o search.o

all: perft stress test xboard

perft: $(SOURCES_CORE) perft.o
	$(CC) $(CFLAGS) -o perft $(SOURCES_CORE) perft.o

stress: $(SOURCES_CORE) stress.o
	$(CC) $(CFLAGS) -o stress $(SOURCES_CORE) stress.o

test: $(SOURCES) test.o
	$(CC) $(CFLAGS) -o test $(SOURCES) test.o

xboard: $(SOURCES) xboard.o
	$(CC) $(CFLAGS) -o xboard $(SOURCES) xboard.o

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

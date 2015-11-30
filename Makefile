
CC=gcc
CFLAGS+=-Wall
CFLAGS+=-O2
CFLSGS+=-g
LDFLAGS+=-pthread
LDLIBS+=-lrt

.PHONY: all clean

all: debug empty loadlib mklib uselib

loadlib: systemf.o
debug: ski-params.o ski-barriers.o ski-test.o ski-hyper.o
empty: ski-params.o ski-barriers.o ski-test.o ski-hyper.o
systemf.o:

uselib: ski-params.o ski-barriers.o ski-hyper.o ski-test.o uselib.o systemf.o

ski-barriers.o: ski-barriers.h
ski-hyper.o: ski-hyper.h
ski-test.o: ski-hyper.h ski-barriers.h
uselib.o: ski-barriers.h ski-hyper.h

clean:
	rm -rf *.o debug empty loadlib mklib uselib

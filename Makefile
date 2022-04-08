#!/usr/bin/make -f

LIBS = $(shell pkg-config --libs glib-2.0)

CFLAG = $(shell pkg-config --cflags glib-2.0)

all:
	mpicc word-count.c $(LIBS) $(CFLAG) -o word-count

clean:
	rm word-count
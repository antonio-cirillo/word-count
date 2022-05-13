#!/usr/bin/make -f

INCLUDES = ./include
LOCAL_LIBS = ./lib

CFLAGS = -I$(INCLUDES) \
	$(shell pkg-config --cflags glib-2.0)

LIBS = $(shell pkg-config --libs glib-2.0)

DEPS = $(wildcard ${INCLUDES}/*.h)
OBJS = $(patsubst %.c, %.o, $(wildcard ${LOCAL_LIBS}/*.c))

main: ${OBJS}
	mpicc ${OBJS} ./src/word-count.c ${LIBS} ${CFLAGS} -o word-count

benchmark: ${OBJS}
	mpicc ${OBJS} ./benchmark/word-count.c ${LIBS} ${CFLAGS} -o word-count

%.o: %.c ${DEPS}
	mpicc -c -o $@ $< ${CFLAGS}

clean:
	rm word-count ${OBJS}
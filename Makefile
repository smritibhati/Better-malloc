CC=gcc
CFLAGS=-Wall

all: mem.c
	${CC} ${CFLAGS} mem.c -o mem

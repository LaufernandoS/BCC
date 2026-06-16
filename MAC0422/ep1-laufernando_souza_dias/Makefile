# Makefile para o EP1

CC = gcc
LDFLAGS = -pthread
READLINE = -lreadline

all: imesh ep1

imesh: imesh.c imesh.h
	$(CC) -o imesh imesh.c $(READLINE)

ep1: ep1.c ep1.h
	$(CC) -o ep1 ep1.c $(LDFLAGS)

clean:
	rm -f imesh ep1 *.o
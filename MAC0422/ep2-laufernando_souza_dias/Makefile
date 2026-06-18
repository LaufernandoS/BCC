# Makefile para o EP2

CC = gcc
LDFLAGS = -pthread

all: ep2

ep2: ep2.c ep2.h
	$(CC) -o ep2 ep2.c $(LDFLAGS)

clean:
	rm -f ep2 *.o
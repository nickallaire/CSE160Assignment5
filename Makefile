CC = gcc
CFLAGS = -g -Wall

default: libcs160mp.a 

cs160mp.o: cs160mp.c cs160mp.h
	$(CC) $(CFLAGS) -c cs160mp.c

msgbench.o: msgbench.c
	$(CC) $(CFLAGS) -c msgbench.c

libcs160mp.a: cs160mp.o msgbench.o
	ar rcs libcs160mp.a cs160mp.o msgbench.o

msgbench: cs160mp.o msgbench.o libcs160mp.a
	cc -o msgbench msgbench.c libcs160mp.a -lpthread

clean:
	-/bin/rm *o libcs160mp.a msgbench

# makefile for common library

all: common.a

install: # nothing

clean:
	rm -f common.a *.o *~

common.a: mcast.o
	ar rcs $@ $<
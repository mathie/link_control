# Nothing important to do in the top directory - just make each
# of the subdirectories

SUBDIRS=common doc include linux_client peer server
TOPDIR := $(shell if [ "$$PWD" != "" ]; then echo $$PWD; else pwd; fi)

CFLAGS=-g -Wall
CPPFLAGS=-DDEBUG -DBROADCAST -I$(TOPDIR)/include -I.

all:
	(set -e; \
	for i in $(SUBDIRS); do \
		if [ -f $$i/Makefile ]; then \
			$(MAKE) CFLAGS="$(CFLAGS)" CPPFLAGS="$(CPPFLAGS)" -C $$i $@; \
		fi; \
	done )

install:
	(set -e; \
	for i in $(SUBDIRS); do \
		if [ -f $$i/Makefile ]; then \
			$(MAKE) CFLAGS="$(CFLAGS)" CPPFLAGS="$(CPPFLAGS)" -C $$i $@; \
		fi; \
	done )

clean:
	(set -e; \
	for i in $(SUBDIRS); do \
		if [ -f $$i/Makefile ]; then \
			$(MAKE) CFLAGS="$(CFLAGS)" CPPFLAGS="$(CPPFLAGS)" -C $$i $@; \
		fi; \
	done )
	rm -f *~ *.o

backup: clean
	(set -e; \
	cd ..; \
	tar zcfv link_control.tar.gz link_control )

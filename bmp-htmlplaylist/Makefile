# $Id: Makefile,v 1.7 2005/04/30 18:41:18 bogorodskiy Exp $

CC = cc
LD = cc
PREFIX ?= /usr/local
INSTALL = install -c -m 755
# You migth want to change it for your system
THREAD_LIB = -pthread

VERSION = "0.2.1"
COMPILEINFO = "Build on `hostname` at `date +\"%Y-%m-%d\"`"

BMP_CFLAGS = `pkg-config --cflags bmp`
BMP_LIBS = `pkg-config --libs bmp`

CFLAGS += -Wall -g $(BMP_CFLAGS) -DVERSION=\"$(VERSION)\" -DCOMPILEINFO=\"$(COMPILEINFO)\"
LDFLAGS = -s
LIBS = $(BMP_LIBS) $(THREAD_LIB)

RM = rm -f

PROJ = bmp-htmlplaylist
OBJS = \
       bmp-htmlplaylist.o

all: $(PROJ)

bmp-htmlplaylist: $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) $(LIBS) -o bmp-htmlplaylist


bmp-htmlplaylist.o: bmp-htmlplaylist.c

.c.o:
	$(CC) $(CFLAGS) -c $*.c

clean:
	$(RM) $(PROJ) $(OBJS) *.core *.html *.xml

install: $(PROJ)
	$(INSTALL) bmp-htmlplaylist $(PREFIX)/bin/

deinstall:
	$(RM) $(PREFIX)/bin/bmp-htmlplaylist
	
dist: clean
	cd .. && tar jcf $(PROJ)-$(VERSION).tar.bz2 $(PROJ)

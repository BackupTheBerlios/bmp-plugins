CC = cc
LD = cc
PREFIX = /usr/local
INSTALL = install -c -m 755
# You migth want to change it for your system
THREAD_LIB = -pthread

VERSION = 0.1

BMP_CFLAGS = `pkg-config --cflags bmp`
BMP_LIBS = `pkg-config --libs bmp`

CFLAGS += -Wall -g $(BMP_CFLAGS) -DVERSION=$(VERSION)
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
	$(RM) $(PROJ) $(OBJS) *.core *.html

install: $(PROJ)
	$(INSTALL) bmp-htmlplaylist $(PREFIX)/bin/

deinstall:
	$(RM) $(PREFIX)/bin/bmp-htmlplaylist
	
dist:
	cd .. && tar zcf $(PROJ)-$(VERSION).tar.bz2 $(PROJ) && \
	md5 $(PROJ)-$(VERSION).tar.bz2 > $(PROJ)-$(VERSION).tar.bz2.sum

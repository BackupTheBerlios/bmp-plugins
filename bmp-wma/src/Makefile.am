SUBDIRS=libffwma wma123

lib_LTLIBRARIES=libwma.la
libdir=	@BEEP_INPUT_PLUGIN_DIR@

libwma_la_LDFLAGS=-module -avoid-version
LIBS=@BEEP_LIBS@ -lffwma
AM_CFLAGS=@BEEP_CFLAGS@ -I$(srcdir)/libffwma -L$(srcdir)/libffwma

libwma_la_SOURCES = bmp-wma.c iir.c iir.h

LDADD=libffwma/libffwma.a

#!/bin/sh

ACLOCAL=aclocal
AUTOCONF=autoconf
AUTOHEADER=autoheader
AUTOMAKE=automake
LIBTOOLIZE=libtoolize

#ACLOCAL_ARGS="-I m4/ -I ac-helpers/"
ACLOCAL_ARGS="-I /usr/local/share/aclocal"

abort () {
    echo "$1 not found or command failed. Aborting!"
    exit 1
}

rm -rf autom4te.cache/
rm -f aclocal.m4 && touch aclocal.m4 && chmod +w aclocal.m4

echo "+ $LIBTOOLIZE"
$LIBTOOLIZE --force --copy --automake || abort "libtoolize"

echo "+ $ACLOCAL $ACLOCAL_ARGS"
$ACLOCAL $ACLOCAL_ARGS 2>/dev/null|| abort "aclocal"

echo "+ $AUTOHEADER"
$AUTOHEADER || abort "autoheader"

echo "+ $AUTOMAKE"
$AUTOMAKE --add-missing --copy --gnu || abort "automake"

echo "+ $AUTOCONF"
$AUTOCONF || abort "autoconf"

exit 0

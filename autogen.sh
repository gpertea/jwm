#!/bin/sh
# Bootstrap for building from a git checkout. Release tarballs ship a
# pre-generated configure script and do not need any of this.
#
# jwm is autoconf-only: Makefile.in is hand-maintained, there is no
# Makefile.am and automake is NOT part of the build. Tools used here:
#   autopoint  (gettext)  - installs po/Makefile.in.in + gettext m4
#   aclocal    (automake) - collects m4 macros (gettext, iconv)
#   automake -ac          - no-op for building; only copies the aux
#                           scripts (install-sh etc.) that configure's
#                           AC_PROG_INSTALL/AM_INIT_AUTOMAKE expect.
#                           It prints a "no Makefile.am" error; ignored.
#   autoconf / autoheader - generate configure and config.h.in

set -e

MACOS_M4_DIR=/opt/homebrew/share/gettext/m4

autopoint --force

if [ -d "$MACOS_M4_DIR" ] ; then
   aclocal -I "$MACOS_M4_DIR"
else
   aclocal
fi

automake -ac 2>/dev/null || :

autoconf
autoheader

touch config.rpath

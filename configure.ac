#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([libeximl2], [0.1.0], [BUG-REPORT-ADDRESS])
AC_CONFIG_SRCDIR([src/parser.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE

AC_PROG_CC_C99
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LIBTOOL

PKG_CHECK_MODULES([libxml2], [libxml-2.0 >= 2.6.0])

AC_ARG_WITH([exip],
	    AS_HELP_STRING([--with-exip=PREFIX], [Build against exip located in PREFIX]))

AS_IF([test "x$with_exip" != "x"], [
	AS_IF([test "x$with_exip" = "xno"], [
	       AC_MSG_ERROR([Cannot build without exip])
	])

	AS_IF([test "x$with_exip" != "xyes"], [
	       CFLAGS+=" -I$with_exip/bin/headers"
	       LDFLAGS+=" -L$with_exip/bin/lib"
	])
])

CFLAGS+=" -Wall -Wextra -O"

AC_CHECK_HEADER([EXIParser.h], [], [AC_MSG_ERROR([Failed to find mandatory header EXIParser.h])])
AC_CHECK_LIB([exip], [initParser], [], [AC_MSG_ERROR([Failed to find mandatory library exip])])

AC_SUBST([libxml2_CFLAGS])
AC_SUBST([libxml2_LIBS])

AC_CONFIG_FILES([Makefile
                 include/Makefile
                 src/Makefile])
AC_OUTPUT

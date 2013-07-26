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

AC_SUBST([libxml2_CFLAGS])
AC_SUBST([libxml2_LIBS])

AC_CONFIG_FILES([Makefile
                 include/Makefile
                 src/Makefile])
AC_OUTPUT

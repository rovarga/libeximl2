AM_CPPFLAGS = -I$(top_srcdir)/include

lib_LTLIBRARIES = libeximl2.la
libeximl2_la_LDFLAGS = -release $(VERSION)
libeximl2_la_CFLAGS = $(CFLAGS) $(libxml2_CFLAGS)
libeximl2_la_LIBADD = $(LIBOBJS) $(libxml2_LIBS)
libeximl2_la_SOURCES = \
		       parser.c


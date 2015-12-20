lib_LTLIBRARIES = libmmspa4pg.la
libmmspa4pg_la_SOURCES = mmspa4pg.c multimodal-twoq.c graphassembler.c
libmmspa4pg_la_LDFLAGS = -version-info 1:0:0
libmmspa4pg_la_LIBADD = -lpq
pkgincludedir = $(includedir)/mmspa4pg
pkginclude_HEADERS = $(top_srcdir)/include/mmspa4pg.h \
					 $(top_srcdir)/include/graphassembler.h \
					 $(top_srcdir)/include/modegraph.h \
					 $(top_srcdir)/include/routingplan.h \
					 $(top_srcdir)/include/pathinfo.h


AM_CPPFLAGS = -I$(top_srcdir)/include @GLIB2_CFLAGS@ -I$(top_srcdir)/lib

bin_PROGRAMS = ccnet-init

ccnet_init_SOURCES = ccnet-init.c

ccnet_init_LDADD = -lssl @LIB_RESOLV@ @GLIB2_LIBS@ \
	$(top_builddir)/lib/libccnet.la

ccnet_init_LDFLAGS = @STATIC_COMPILE@ @CONSOLE@ @SERVER_PKG_RPATH@


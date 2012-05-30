
AM_CFLAGS = -DPKGDATADIR=\"$(pkgdatadir)\" \
	-DPACKAGE_DATA_DIR=\""$(pkgdatadir)"\" \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/lib  \
	@GLIB2_CFLAGS@ \
	-I/usr/include/python$(PYTHON_VERSION) -I.

pkgpyexec_PYTHON = __init__.py pool.py rpc.py

pkgpyexec_LTLIBRARIES = cevent.la cclient.la

cevent_la_LDFLAGS = -module -avoid-version -export-symbols-regex initcevent -no-undefined
cevent_la_SOURCES = cevent-module.c
cevent_la_LIBADD = @GLIB2_LIBS@  ../../lib/libccnet.la @LIB_PYTHON@

cclient_la_LDFLAGS = -module -avoid-version -export-symbols-regex initcclient -no-undefined
cclient_la_SOURCES = cclient-module.c
cclient_la_LIBADD = @GLIB2_LIBS@  ../../lib/libccnet.la @LIB_PYTHON@

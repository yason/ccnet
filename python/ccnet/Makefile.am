
AM_CFLAGS = -DPKGDATADIR=\"$(pkgdatadir)\" \
	-DPACKAGE_DATA_DIR=\""$(pkgdatadir)"\" \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/lib  \
	@GLIB2_CFLAGS@ \
    @PYTHON_INCLUDES@ -I.


pkgpyexec_PYTHON = __init__.py pool.py rpc.py

pkgpyexec_LTLIBRARIES = cclient.la

cclient_la_LDFLAGS = -module -avoid-version -export-symbols-regex initcclient -no-undefined @SERVER_PKG_PY_RPATH@
cclient_la_SOURCES = cclient-module.c
cclient_la_LIBADD = @GLIB2_LIBS@ $(top_builddir)/lib/libccnet.la @LIB_PYTHON@

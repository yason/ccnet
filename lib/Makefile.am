
AM_CPPFLAGS = @GLIB2_CFLAGS@ -I$(top_srcdir)/include \
	-I$(top_srcdir)/include/ccnet \
	-I$(top_srcdir)/lib \
	-I$(top_builddir)/include \
	-DG_LOG_DOMAIN=\"Ccnet\" \
	-DCCNET_LIB \
	@SEARPC_CFLAGS@ \
	@MSVC_CFLAGS@ \
	-Wall

BUILT_SOURCES = gensource

lib_LTLIBRARIES = libccnet.la

noinst_HEADERS = buffer.h \
	string-array.h \
	packet-io.h  \
	include.h \
	marshal.h \
	net.h \
	peer-common.h \
	string-util.h \
	config-db.h \
	utils.h \
	ccnet-object.h \
	bloom-filter.h \
	db.h rsa.h \
	rpc-common.h

ccnetincludedir = $(includedir)/ccnet
ccnetinclude_DATA = ccnet-object.h

libccnet_la_SOURCES = ccnet-client.c packet-io.c utils.c db.c \
	message.c proc-factory.c \
	processor.c buffer.c \
	peer.c sendcmd-proc.c \
	mqclient-proc.c invoke-service-proc.c \
	string-array.c \
	marshal.c  rsa.c \
	bloom-filter.c \
	mainloop.c net.c cevent.c timer.c ccnet-session-base.c job-mgr.c \
	rpcserver-proc.c ccnetrpc-transport.c threaded-rpcserver-proc.c \
	ccnetobj.c \
	async-rpc-proc.c ccnet-rpc-wrapper.c \
	config-db.c \
	client-pool.c

EXTRA_DIST = ccnetobj.vala rpc_table.py

libccnet_la_LDFLAGS = -no-undefined -version-info 0:0:0
libccnet_la_LIBADD = -lpthread @GLIB2_LIBS@  @GOBJECT_LIBS@ -lssl -lcrypto @LIB_GDI32@ \
				     @LIB_UUID@ @LIB_WS32@ @LIB_PSAPI@ -lsqlite3 \
					 -levent @SEARPC_LIBS@ @LIB_SHELL32@


noinst_LTLIBRARIES = libccnetd.la

libccnetd_la_SOURCES = utils.c db.c job-mgr.c \
	rsa.c bloom-filter.c marshal.c net.c timer.c ccnet-session-base.c \
	ccnetobj.c

libccnetd_la_LDFLAGS = -no-undefined
libccnetd_la_LIBADD = @GLIB2_LIBS@  @GOBJECT_LIBS@ -lssl -lcrypto @LIB_GDI32@ \
	                  -lsqlite3 -levent @LIB_WS32@ @LIB_UUID@ \
					  @LIB_SHELL32@ @LIB_PSAPI@ @SEARPC_LIBS@


ccnet_object_define = ccnetobj.vala

ccnet-client.c: ccnet-object.h

ccnet-object.h: ${ccnet_object_define}
	rm -f $@
	valac --pkg posix ${ccnet_object_define} -C -H ccnet-object.h

ccnetobj.c: ${ccnet_object_define}
	rm -f $@
	valac -C --pkg posix ${ccnet_object_define}

searpc_gen = searpc-signature.h searpc-marshal.h

gensource: ${searpc_gen}

${searpc_gen}: $(top_srcdir)/lib/rpc_table.py
	@echo "[libsearpc]: generating rpc header files"
	@PYTHON@ `which searpc-codegen.py` $(top_srcdir)/lib/rpc_table.py
	@echo "[libsearpc]: done"

clean-local:
	-rm ${searpc_gen} 

CLEANFILES = ccnetobj.c ccnet-object.h ${searpc_gen}

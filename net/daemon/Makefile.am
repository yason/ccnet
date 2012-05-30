
AM_CPPFLAGS = @GLIB2_CFLAGS@ @GOBJECT_CFLAGS@ \
	-DCCNET_DAEMON \
	-I$(top_srcdir)/net/common \
	-I$(top_srcdir)/include -I$(top_srcdir)/include/ccnet \
	-I$(top_srcdir)/lib \
	-I$(top_builddir)/include \
	@SEARPC_CFLAGS@ \
	-Wall

bin_PROGRAMS = ccnet

PROC_HEADER_FILES = $(addprefix ../common/processors/, \
	rcvmsg-proc.h \
	sendmsg-proc.h \
	rcvcmd-proc.h \
	getpubinfo-proc.h putpubinfo-proc.h \
	keepalive2-proc.h \
	mqserver-proc.h \
	service-proxy-proc.h service-stub-proc.h \
	rpcserver-proc.h \
	echo-proc.h )

# sync-kvitem-proc.h sync-kvitem-slave-proc.h
# 	getmsg-proc.h putmsg-proc.h \
# rcvevent-proc.h \
#	get-userpeer-proc.h \
#	update-user-proc.h \
#	update-pubinfo-proc.h \
#	getgroupinfo-proc.h putgroupinfo-proc.h \
#	sendgroupinfo-proc.h recvgroupinfo-proc.h \
#	getaddrinfo-proc.h putaddrinfo-proc.h \
#	sync-relay-proc.h sync-relay-slave-proc.h \
#	sync-role-proc.h \
#	kvserver-proc.h \
#	sample-master-proc.h sample-slave-proc.h \
#	send-profile-proc.h

common_headers = ../common/algorithms.h \
	../common/proc-factory.h ../common/session.h \
	../common/common.h ../common/handshake.h ../common/perm-mgr.h \
	../common/peer.h ../common/connect-mgr.h \
	../common/packet-io.h ../common/ccnet-config.h \
	../common/log.h ../common/peer-mgr.h \
	../common/message.h \
	../common/getgateway.h ../common/message-manager.h \
	../common/processor.h \
	../common/peermgr-message.h \
	../common/list.h ../common/rpc-service.h \
	 processors/sendlogin-proc.h processors/sendlogout-proc.h \
	../common/ccnet-db.h

# ../common/group.h
# ../common/group-mgr.h \
# ../common/groupmgr-message.h
# ../common/info-syncher.h

noinst_HEADERS = $(common_headers) \
	$(PROC_HEADER_FILES)

common_srcs = ../common/session.c ../common/peer-mgr.c ../common/packet-io.c \
	../common/message.c ../common/perm-mgr.c \
	../common/log.c ../common/peer.c ../common/algorithms.c \
	../common/handshake.c ../common/processor.c \
	../common/getgateway.c ../common/connect-mgr.c \
	../common/message-manager.c \
	../common/proc-factory.c \
	../common/ccnet-config.c \
	../common/rpc-service.c \
	../common/peermgr-message.c \
	processors/sendlogin-proc.c processors/sendlogout-proc.c \
	../common/processors/sendmsg-proc.c ../common/processors/rcvmsg-proc.c \
	../common/processors/rcvcmd-proc.c \
	../common/processors/getpubinfo-proc.c \
	../common/processors/putpubinfo-proc.c \
	../common/processors/keepalive2-proc.c \
	../common/processors/mqserver-proc.c \
	../common/processors/service-proxy-proc.c \
	../common/processors/service-stub-proc.c \
	../common/processors/rpcserver-proc.c \
	../common/processors/echo-proc.c

ccnet_SOURCES = ccnet-daemon.c \
	$(common_srcs)

# ../common/group.c ../common/group-mgr.c \
# ../common/info-syncher.c \
#  ../common/groupmgr-message.c \
# kvitem-manager.c
# processors/kvserver-proc.c
# processors/sync-kvitem-proc.c processors/sync-kvitem-slave-proc.c
# processors/sync-transitem-slave-proc.c 
# processors/tunnel-proc.c processors/tunnel-slave-proc.c
# processors/put-transinfo-proc.c
# processors/sync-relay-proc.c processors/sync-relay-slave-proc.c
# 	../common/processors/getmsg-proc.c ../common/processors/putmsg-proc.c \
# ../common/processors/rcvevent-proc.c \
# ../common/processors/update-user-proc.c \
# ../common/processors/update-pubinfo-proc.c \
# ../common/processors/getgroupinfo-proc.c \
# ../common/processors/putgroupinfo-proc.c \
# ../common/processors/sendgroupinfo-proc.c \
# ../common/processors/recvgroupinfo-proc.c \
# ../common/processors/putaddrinfo-proc.c \
# ../common/processors/getaddrinfo-proc.c \
# 	../common/processors/sync-role-proc.c


ccnet_LDADD = -levent $(top_builddir)/lib/libccnetd.la \
           @GLIB2_LIBS@ @GOBJECT_LIBS@ -lssl @LIB_RT@ @LIB_UUID@ -lsqlite3 \
           @LIB_WS32@ @LIB_INTL@ @LIB_IPHLPAPI@ @SEARPC_LIBS@


ccnet_LDFLAGS = @STATIC_COMPILE@ @CONSOLE@ -no-undefined

check_PROGRAMS = ccnet-test

ccnet_test_SOURCES = ccnet-test.c $(common_srcs)

ccnet_test_LDADD = -levent $(top_builddir)/lib/libccnetd.la \
	@GLIB2_LIBS@ @GOBJECT_LIBS@  -lssl @LIB_RT@ @LIB_UUID@ -lsqlite3 \
	@LIB_WS32@ @LIB_INTL@ @LIB_IPHLPAPI@ @SEARPC_LIBS@

ccnet_test_LDFLAGS = @STATIC_COMPILE@ -no-undefined @CONSOLE@

TESTS = ccnet-test



AM_CPPFLAGS = @GLIB2_CFLAGS@ @GOBJECT_CFLAGS@ \
	-DCCNET_SERVER \
	-I$(top_srcdir)/net/common \
	-I$(top_srcdir)/include -I$(top_srcdir)/include/ccnet \
	-I$(top_srcdir)/lib \
	-I$(top_builddir)/include \
	@SEARPC_CFLAGS@ \
	@MYSQL_CFLAGS@ \
	@ZDB_CFLAGS@ \
	-Wall

bin_PROGRAMS = ccnet-server

PROC_HEADER_FILES =  $(addprefix processors/, \
	recvlogin-proc.h recvlogout-proc.h) \
	$(addprefix ../common/processors/, \
	rcvmsg-proc.h \
	rcvcmd-proc.h \
	sendmsg-proc.h \
	getpubinfo-proc.h putpubinfo-proc.h \
	keepalive2-proc.h \
	mqserver-proc.h \
	service-proxy-proc.h service-stub-proc.h \
	rpcserver-proc.h )

# sync-kvitem-proc.h sync-kvitem-slave-proc.h 
#	update-user-slave-proc.h \
#	update-pubinfo-slave-proc.h \
# put-userpeer-proc.h
# 	getmsg-proc.h putmsg-proc.h \
# rcvevent-proc.h \
#	get-userpeer-proc.h  \
#	update-user-proc.h \
#	update-pubinfo-proc.h \
#	getgroupinfo-proc.h putgroupinfo-proc.h \
#	sendgroupinfo-proc.h recvgroupinfo-proc.h \
#	getaddrinfo-proc.h putaddrinfo-proc.h \
# 	sync-relay-proc.h sync-relay-slave-proc.h \
#	sync-role-proc.h \
#	kvserver-proc.h \
#	sample-master-proc.h sample-slave-proc.h \
#	receive-profile-proc.h \
#	send-profile-proc.h

common_headers = 
	../common/algorithms.h \
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
	../common/ccnet-db.h

# relay-mgr.h
# ../common/group-mgr.h \
# ../common/group.h
# ../common/info-syncher.h \
# ../common/groupmgr-message.h

noinst_HEADERS = $(common_headers) \
	user-mgr.h group-mgr.h \
	$(PROC_HEADER_FILES)


common_srcs = ../common/ccnet-db.c \
	../common/session.c ../common/peer-mgr.c ../common/packet-io.c \
	../common/message.c ../common/perm-mgr.c \
	../common/log.c ../common/peer.c ../common/algorithms.c \
	../common/handshake.c ../common/processor.c \
	../common/getgateway.c ../common/connect-mgr.c \
	../common/message-manager.c \
	../common/proc-factory.c \
	../common/ccnet-config.c \
	../common/rpc-service.c \
	../common/peermgr-message.c \
	../common/processors/sendmsg-proc.c ../common/processors/rcvmsg-proc.c \
	../common/processors/rcvcmd-proc.c \
	../common/processors/putpubinfo-proc.c \
	../common/processors/getpubinfo-proc.c \
	../common/processors/keepalive2-proc.c \
	../common/processors/mqserver-proc.c \
	../common/processors/service-proxy-proc.c \
	../common/processors/service-stub-proc.c \
	../common/processors/rpcserver-proc.c

ccnet_server_SOURCES = ccnet-server.c \
	user-mgr.c group-mgr.c \
	processors/recvlogin-proc.c processors/recvlogout-proc.c \
	$(common_srcs)

# relay-mgr.c
# kvitem-manager.c
# processors/kvserver-proc.c
# processors/sync-kvitem-proc.c processors/sync-kvitem-slave-proc.c
# processors/sync-transitem-slave-proc.c 
# processors/tunnel-proc.c processors/tunnel-slave-proc.c
# processors/put-transinfo-proc.c
# transfer-mgr.c transfer-item.c
# ../common/group.c
# ../common/group-mgr.c \
# ../common/groupmgr-message.c \
# ../common/info-syncher.c \
# processors/sync-relay-proc.c processors/sync-relay-slave-proc.c
# processors/sync-role-slave-proc.c
# 	processors/put-userpeer-proc.c \
#	processors/update-user-slave-proc.c \
#	processors/update-pubinfo-slave-proc.c \
# 	../common/processors/getmsg-proc.c ../common/processors/putmsg-proc.c \
# ../common/processors/rcvevent-proc.c \
#	../common/processors/get-userpeer-proc.c \
#	../common/processors/update-user-proc.c \
#	../common/processors/update-pubinfo-proc.c \
#	../common/processors/getgroupinfo-proc.c \
#	../common/processors/putgroupinfo-proc.c \
#	../common/processors/sendgroupinfo-proc.c \
#	../common/processors/recvgroupinfo-proc.c \
#	../common/processors/putaddrinfo-proc.c \
#	../common/processors/getaddrinfo-proc.c \
# 	../common/processors/sync-role-proc.c \
# 	processors/receive-profile-proc.c

ccnet_server_LDADD = -levent $(top_builddir)/lib/libccnetd.la \
           @GLIB2_LIBS@ @GOBJECT_LIBS@ -lssl @LIB_RT@ @LIB_UUID@ -lsqlite3 \
           @LIB_WS32@ @LIB_INTL@ @LIB_IPHLPAPI@ @SEARPC_LIBS@ @ZDB_LIBS@


ccnet_server_LDFLAGS = @STATIC_COMPILE@ @CONSOLE@ @SERVER_PKG_RPATH@ -no-undefined


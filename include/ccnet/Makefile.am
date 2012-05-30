
ccnetdir = $(includedir)/ccnet

ccnet_HEADERS = ccnet-client.h peer.h proc-factory.h \
	message.h option.h \
	processor.h sendcmd-proc.h \
	mqclient-proc.h invoke-service-proc.h \
	buildin-types.h \
	status-code.h cevent.h timer.h ccnet-session-base.h \
	valid-check.h

EXTRA_DIST = buildin-types.h.template

buildin_type_headers = ccnet-client.h

# Extract enums for porting to python
buildin-types.h: $(buildin_type_headers) buildin-types.h.template
	glib-mkenums --template $(top_srcdir)/include/ccnet/buildin-types.h.template \
		$(buildin_type_headers) >> xgen-gtbh \
	&& cp xgen-gtbh buildin-types.h \
	&& rm -f xgen-gtbh

DISTCLEANFILES = buildin-types.h
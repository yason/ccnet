/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "include.h"
#include <stdio.h>

#include "kvitem.h"
#include "kvclient-proc.h"

#define SC_ITEM "300"
#define SC_UNSUBSCRIBE "301"


static int kv_client_start (CcnetProcessor *processor, int argc, char **argv);

static void handle_response (CcnetProcessor *processor,
                             char *code, char *code_msg,
                             char *content, int clen);


G_DEFINE_TYPE (CcnetKvclientProc, ccnet_kvclient_proc, CCNET_TYPE_PROCESSOR)

static void
ccnet_kvclient_proc_class_init (CcnetKvclientProcClass *klass)
{
    CcnetProcessorClass *proc_class = CCNET_PROCESSOR_CLASS (klass);

    proc_class->start = kv_client_start;
    proc_class->handle_response = handle_response;
    proc_class->name = "kvclient-proc";
}

static void
ccnet_kvclient_proc_init (CcnetKvclientProc *processor)
{
}

static int
kv_client_start (CcnetProcessor *processor, int argc, char **argv)
{
    char buf[256];

    snprintf (buf, 256, "kvserver %s", argv[0]);

    ccnet_processor_send_request (processor, buf);

    return 0;
}

static void handle_response (CcnetProcessor *processor,
                             char *code, char *code_msg,
                             char *content, int clen)
{
    CcnetKvclientProc *proc = (CcnetKvclientProc *) processor;
    CcnetKVItem *item;

    if (content[clen-1] != '\0') {
        ccnet_warning ("Bad format\n");
        return;
    }

    if (strncmp(code, "300", 3) == 0) {
        item = ccnet_kvitem_from_string (content);
        if (proc->kvitem_got_cb)
            proc->kvitem_got_cb (item, proc->cb_data);
        ccnet_kvitem_unref (item);
    }
}

void ccnet_kvclient_proc_set_item_got_cb (CcnetKvclientProc *processor,
                                          KvitemGotCB callback,
                                          void *cb_data)
{
    processor->kvitem_got_cb = callback;
    processor->cb_data = cb_data;
}

void ccnet_kvclient_proc_put_item (CcnetKvclientProc *proc,
                                   CcnetKVItem *kvitem)
{
    CcnetProcessor *processor = (CcnetProcessor *) proc;
    GString *buf;

    buf = g_string_new (NULL);

    ccnet_kvitem_to_string_buf (kvitem, buf);
    ccnet_processor_send_update (processor, SC_ITEM, NULL, buf->str, buf->len+1);
    g_string_free (buf, TRUE);
}

/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <json-glib/json-glib.h>

#include <stdio.h>
 
#include "ccnet-client.h"
#include "sendevent-proc.h"

enum {
    INIT,
    REQUEST_SENT,
    CONNECTED
};

typedef struct  {
    CcnetEvent *event;
} CcnetSendeventProcPriv;

#define GET_PRIV(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), CCNET_TYPE_SENDEVENT_PROC, CcnetSendeventProcPriv))

#define USE_PRIV \
    CcnetSendeventProcPriv *priv = GET_PRIV(processor);


G_DEFINE_TYPE (CcnetSendeventProc, ccnet_sendevent_proc, CCNET_TYPE_PROCESSOR)

static int start (CcnetProcessor *processor, int argc, char **argv);
static void handle_response (CcnetProcessor *processor,
                             char *code, char *code_msg,
                             char *content, int clen);

static void
release_resource(CcnetProcessor *processor)
{
    USE_PRIV;

    if (priv->event)
        g_object_unref (priv->event);
    CCNET_PROCESSOR_CLASS (ccnet_sendevent_proc_parent_class)->release_resource (processor);
}


static void
ccnet_sendevent_proc_class_init (CcnetSendeventProcClass *klass)
{
    CcnetProcessorClass *proc_class = CCNET_PROCESSOR_CLASS (klass);

    proc_class->start = start;
    proc_class->handle_response = handle_response;
    proc_class->release_resource = release_resource;
    proc_class->name = "sendevent-proc";

    g_type_class_add_private (klass, sizeof (CcnetSendeventProcPriv));
}

static void
ccnet_sendevent_proc_init (CcnetSendeventProc *processor)
{
}


static int
start (CcnetProcessor *processor, int argc, char **argv)
{
    ccnet_client_send_request (processor->session,
                            REQUEST_ID (processor->id), "receive-event");
    processor->state = REQUEST_SENT;
    return 0;
}

int
ccnet_sendevent_proc_set_event (CcnetSendeventProc *sendevent_proc, 
                                CcnetEvent *event)
{
    CcnetSendeventProcPriv *priv = GET_PRIV (sendevent_proc);

    priv->event = event;
    g_object_ref (event);
    
    return 0;
}

static void
handle_response (CcnetProcessor *processor,
                 char *code, char *code_msg,
                 char *content, int clen)
{
    USE_PRIV;

    switch (processor->state) {
    case REQUEST_SENT:
        processor->state = CONNECTED;
        gsize len = 0;
        gchar *event_str = json_gobject_to_data ((GObject *)(priv->event), &len);
        ccnet_client_send_update (processor->session, UPDATE_ID(processor->id), 
                                  "200", NULL, 
                                  event_str, len+1); /* including '\0' */
        g_free (event_str);

        break;
    case CONNECTED:
        ccnet_processor_done (processor, TRUE);
        break;
    default:
        break;
    }

}

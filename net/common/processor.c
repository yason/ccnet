/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */


#include "common.h"

#include "timer.h"
#include "peer.h"
#include "processor.h"
#include "session.h"
#include "connect-mgr.h"
#include "proc-factory.h"
#include "utils.h"

#include "processors/keepalive2-proc.h"
#include "processors/service-proxy-proc.h"
#include "processors/service-stub-proc.h"

#define DEBUG_FLAG  CCNET_DEBUG_PROCESSOR
#include "log.h"

enum {
    DONE_SIG,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (CcnetProcessor, ccnet_processor, G_TYPE_OBJECT);

static void default_shutdown (CcnetProcessor *processor);
static void default_release_resource (CcnetProcessor *processor);

static void
ccnet_processor_class_init (CcnetProcessorClass *klass)
{
    /* GObjectClass *gobject_class = G_OBJECT_CLASS (klass); */

    klass->start = NULL;
    klass->handle_update = NULL;
    klass->handle_response = NULL;
    klass->shutdown = default_shutdown;
    klass->release_resource = default_release_resource;

    signals[DONE_SIG] = 
        g_signal_new ("done", CCNET_TYPE_PROCESSOR, 
                      G_SIGNAL_RUN_LAST,
                      0,        /* no class singal handler */
                      NULL, NULL, /* no accumulator */
                      g_cclosure_marshal_VOID__BOOLEAN,
                      G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
}

static void
ccnet_processor_init (CcnetProcessor *processor)
{
    
}

int ccnet_processor_start (CcnetProcessor *processor, int argc, char **argv)
{
    /* set this value to now even if this is a master processor and
       has not received any packet yet for simplifying the keepalive
       logic. */
    g_assert (processor->start_time == 0);

    time_t now = time(NULL);
    processor->start_time = now;
    if (IS_SLAVE(processor))
        processor->t_packet_recv = now;
    else
        processor->t_packet_recv = 0;

    processor->failure = PROC_NOTSET;
    if (processor->peer->net_state != PEER_CONNECTED) {
        if (IS_SLAVE(processor)) {
            ccnet_processor_send_response (processor, SC_NETDOWN, SS_NETDOWN,
                                           NULL, 0);
        }
        ccnet_processor_shutdown (processor, PROC_NETDOWN);
        return -1;
    }

    return CCNET_PROCESSOR_GET_CLASS (processor)->start (
        processor, argc, argv);
}

int ccnet_processor_startl (CcnetProcessor *processor, ...)
{
    va_list ap;
    int argc = 0;
    char **argv = g_malloc (sizeof(char *) * 10);
    char *arg;
    int max = 10;
    int ret;
    
    va_start (ap, processor);
    arg = va_arg (ap, char *);
    while (arg) {
        if (argc >= max) {
            max *= 2;
            argv = g_realloc (argv, sizeof(char *) * max);
        }
        argv[argc++] = arg;

        arg = va_arg (ap, char *);        
    }
    va_end (ap);
    
    ret = ccnet_processor_start (processor, argc, argv);
    g_free (argv);

    return ret;
}

static void default_shutdown (CcnetProcessor *processor)
{
    processor->err_code = ERR_INTR;
}

static void default_release_resource(CcnetProcessor *processor)
{
    if (processor->retry_timer)
        ccnet_timer_free (&processor->retry_timer);

    g_free (processor->name);
    if (processor->peer) {
        g_object_unref (processor->peer);
        processor->peer = NULL;
    }
}

/* should be called before recycle */
void
ccnet_processor_release_resource(CcnetProcessor *processor)
{
    CCNET_PROCESSOR_GET_CLASS (processor)->release_resource(processor);
}

void
ccnet_processor_shutdown (CcnetProcessor *processor, int reason)
{
    if (processor->state == STATE_IN_SHUTDOWN) {
        return;
    }

    if (processor->failure == PROC_NOTSET && reason != PROC_NOTSET)
        processor->failure = reason;
    processor->state = STATE_IN_SHUTDOWN;
    ccnet_debug ("[Proc] Shutdown %s(%d) of peer %s\n",
                 GET_PNAME(processor), PRINT_ID(processor->id), 
                 processor->peer->name);
    CCNET_PROCESSOR_GET_CLASS (processor)->shutdown (processor);

    if (reason == PROC_DONE)
        g_signal_emit (processor, signals[DONE_SIG], 0, TRUE);
    else
        g_signal_emit (processor, signals[DONE_SIG], 0, FALSE);

    if (!processor->peer->in_shutdown) {
        ccnet_peer_remove_processor (processor->peer, processor);
    }
    ccnet_processor_release_resource (processor);

    ccnet_proc_factory_recycle (processor->session->proc_factory, processor);
}

void
ccnet_processor_done (CcnetProcessor *processor, gboolean success)
{
    if (processor->state == STATE_IN_SHUTDOWN) {
        return;
    }

    processor->state = STATE_IN_SHUTDOWN;
    if (processor->failure == PROC_NOTSET)
        processor->failure = PROC_DONE;
    if (!processor->peer->is_local)
        ccnet_debug ("Processsor %s(%d) done %d\n", GET_PNAME(processor),
                     PRINT_ID(processor->id), success);

    if (processor->peer && !processor->peer->in_shutdown && success)
    {
        if (!IS_SLAVE (processor)) {
            ccnet_processor_send_update (processor, SC_PROC_DONE, SS_PROC_DONE,
                                         NULL, 0);
        }
    }

    /* When we emit the done signal, the corresponding handler may
     * shutdown the peer, we should remove this processor from the
     * peers processor list, otherwise this processor will be freed
     * twice. */
    g_signal_emit (processor, signals[DONE_SIG], 0, success);

    if (!processor->peer->in_shutdown) {
        ccnet_peer_remove_processor (processor->peer, processor);
    }

    ccnet_processor_release_resource (processor);

    ccnet_proc_factory_recycle (processor->session->proc_factory, processor);
}


void
ccnet_processor_error (CcnetProcessor *processor,
                       const char *error_code,
                       const char *error_string)
{
    ccnet_processor_send_response (
        processor, error_code, error_string, NULL, 0);
    ccnet_processor_done (processor, FALSE);
}

static void ccnet_processor_keep_alive_response (CcnetProcessor *processor);

void ccnet_processor_handle_update (CcnetProcessor *processor, 
                                    char *code, char *code_msg,
                                    char *content, int clen)
{
    if (code[0] == '5' || code[0] == '4') {
        ccnet_warning ("[Proc] Shutdown processor %s(%d) for bad update: %s %s\n",
                       GET_PNAME(processor), PRINT_ID(processor->id),
                       code, code_msg);

        /* Proxy proc should relay the message before it shuts down. */
        if (CCNET_IS_SERVICE_PROXY_PROC(processor) ||
            CCNET_IS_SERVICE_STUB_PROC(processor)) {
            CCNET_PROCESSOR_GET_CLASS (processor)->handle_update (
                processor, code, code_msg, content, clen);
        }

        if (memcmp(code, SC_UNKNOWN_SERVICE, 3) == 0)
            ccnet_processor_shutdown (processor, PROC_NO_SERVICE);
        else if (memcmp(code, SC_PERM_ERR, 3) == 0)
            ccnet_processor_shutdown (processor, PROC_PERM_ERR);
        else if (memcmp(code, SC_CON_TIMEOUT, 3) == 0)
            ccnet_processor_shutdown (processor, PROC_CON_TIMEOUT);
        else if (memcmp(code, SC_KEEPALIVE_TIMEOUT, 3) == 0)
            ccnet_processor_shutdown (processor, PROC_TIMEOUT);
        else if (memcmp(code, SC_NETDOWN, 3) == 0)
            ccnet_processor_shutdown (processor, PROC_NETDOWN);
        else
            ccnet_processor_shutdown (processor, PROC_BAD_RESP);
        return;
    }

    processor->t_packet_recv = time(NULL);

    if (memcmp (code, SC_PROC_KEEPALIVE, 3) == 0) {
        ccnet_processor_keep_alive_response (processor);
    } else if (memcmp (code, SC_PROC_ALIVE, 3) == 0) {
        /* ccnet_debug ("[Proc] received alive update (%d)\n",
           PRINT_ID(processor->id)); 
        */
        /* do nothing */
    } else if (memcmp (code, SC_PROC_DEAD, 3) == 0) {
        ccnet_debug ("[Proc] Shutdown processor %s(%d) when remote processor dies\n",
                     GET_PNAME(processor), PRINT_ID(processor->id));

        if (CCNET_IS_SERVICE_PROXY_PROC(processor) ||
            CCNET_IS_SERVICE_STUB_PROC(processor)) {
            CCNET_PROCESSOR_GET_CLASS (processor)->handle_update (
                processor, code, code_msg, content, clen);
        }

        ccnet_processor_shutdown (processor, PROC_REMOTE_DEAD);
    } else if (memcmp (code, SC_PROC_DONE, 3) == 0) {
        ccnet_debug ("[Proc] Shutdown processor %s(%d) when master done\n",
                     GET_PNAME(processor), PRINT_ID(processor->id));

        if (CCNET_IS_SERVICE_PROXY_PROC(processor) ||
            CCNET_IS_SERVICE_STUB_PROC(processor)) {
            CCNET_PROCESSOR_GET_CLASS (processor)->handle_update (
                processor, code, code_msg, content, clen);
        }

        ccnet_processor_done (processor, TRUE);
    } else
        CCNET_PROCESSOR_GET_CLASS (processor)->handle_update (processor, 
                                                              code, code_msg, 
                                                              content, clen);
}

void ccnet_processor_handle_response (CcnetProcessor *processor, 
                                      char *code, char *code_msg,
                                      char *content, int clen)
{
    if ((code[0] == '5' || code[0] == '4') &&
        !CCNET_IS_KEEPALIVE2_PROC(processor))
    {
        ccnet_warning ("[Proc] peer %.10s, Shutdown processor %s(%d) for bad response: %s %s\n",
                       processor->peer->id, GET_PNAME(processor), PRINT_ID(processor->id),
                       code, code_msg);

        /* Stub proc should relay the message before it shuts down. */
        if (CCNET_IS_SERVICE_PROXY_PROC(processor) ||
            CCNET_IS_SERVICE_STUB_PROC(processor)) {
            CCNET_PROCESSOR_GET_CLASS (processor)->handle_response (
                processor, code, code_msg, content, clen);
        }

        if (memcmp(code, SC_UNKNOWN_SERVICE, 3) == 0)
            ccnet_processor_shutdown (processor, PROC_NO_SERVICE);
        else if (memcmp(code, SC_PERM_ERR, 3) == 0)
            ccnet_processor_shutdown (processor, PROC_PERM_ERR);
        else if (memcmp(code, SC_CON_TIMEOUT, 3) == 0)
            ccnet_processor_shutdown (processor, PROC_CON_TIMEOUT);
        else if (memcmp(code, SC_KEEPALIVE_TIMEOUT, 3) == 0)
            ccnet_processor_shutdown (processor, PROC_TIMEOUT);
        else if (memcmp(code, SC_NETDOWN, 3) == 0)
            ccnet_processor_shutdown (processor, PROC_NETDOWN);
        else
            ccnet_processor_shutdown (processor, PROC_BAD_RESP);
        return;
    }

    processor->t_packet_recv = time(NULL);

    if (memcmp (code, SC_PROC_KEEPALIVE, 3) == 0) {
        ccnet_processor_keep_alive_response (processor);
    } else if (memcmp (code, SC_PROC_ALIVE, 3) == 0) {
        /* do nothing */
    } else if (memcmp (code, SC_PROC_DEAD, 3) == 0) {
        ccnet_debug ("[Proc] Shutdown processor %s(%d) when remote processor dies\n",
                     GET_PNAME(processor), PRINT_ID(processor->id));

        if (CCNET_IS_SERVICE_PROXY_PROC(processor) ||
            CCNET_IS_SERVICE_STUB_PROC(processor)) {
            CCNET_PROCESSOR_GET_CLASS (processor)->handle_response (
                processor, code, code_msg, content, clen);
        }

        ccnet_processor_shutdown (processor, PROC_REMOTE_DEAD);
    } else
        CCNET_PROCESSOR_GET_CLASS (processor)->handle_response (processor,
                                                                code, code_msg, 
                                                                content, clen);
}

void ccnet_processor_handle_sigchld (CcnetProcessor *processor, int status)
{
    CCNET_PROCESSOR_GET_CLASS (processor)->handle_sigchld (processor, 
                                                           status);
}

void
ccnet_processor_send_request (CcnetProcessor *processor,
                              const char *request)
{
    ccnet_peer_send_request (processor->peer, REQUEST_ID (processor->id), 
                             request);
}

void
ccnet_processor_send_request_l (CcnetProcessor *processor, ...)
{
    va_list ap;
    GString *buf = g_string_new(NULL);
    char *arg;
    
    va_start (ap, processor);
    arg = va_arg (ap, char *);
    while (arg) {
        g_string_append (buf, arg);
        arg = va_arg (ap, char *);        
    }
    va_end (ap);

    ccnet_peer_send_request (processor->peer,
                             REQUEST_ID (processor->id), buf->str); 
    g_string_free (buf, TRUE);
}


void
ccnet_processor_send_update (CcnetProcessor *processor,
                             const char *code,
                             const char *code_msg,
                             const char *content, int clen)
{
    ccnet_peer_send_update (processor->peer, UPDATE_ID(processor->id),
                            code, code_msg, content, clen);
}

void
ccnet_processor_send_error_update (CcnetProcessor *processor, 
                                   const char *code,
                                   const char *code_msg)
{
    ccnet_peer_send_update (processor->peer, UPDATE_ID(processor->id),
                            code, code_msg, NULL, 0);
}

void
ccnet_processor_send_response (CcnetProcessor *processor,
                             const char *code,
                             const char *code_msg,
                             const char *content, int clen)
{
    ccnet_peer_send_response (processor->peer, RESPONSE_ID (processor->id), 
                              code, code_msg, content, clen);
}

void ccnet_processor_keep_alive (CcnetProcessor *processor)
{
    if (IS_SLAVE (processor))
        ccnet_processor_send_response (processor, SC_PROC_KEEPALIVE, 
                                       SS_PROC_KEEPALIVE, NULL, 0);
    else
        ccnet_processor_send_update (processor, SC_PROC_KEEPALIVE, 
                                     SS_PROC_KEEPALIVE, NULL, 0);
    processor->t_keepalive_sent = time (NULL);
}

static void ccnet_processor_keep_alive_response (CcnetProcessor *processor)
{
    if (IS_SLAVE (processor))
        ccnet_processor_send_response (processor, SC_PROC_ALIVE, 
                                       SS_PROC_ALIVE, NULL, 0);
    else
        ccnet_processor_send_update (processor, SC_PROC_ALIVE, 
                                     SS_PROC_ALIVE, NULL, 0);
}

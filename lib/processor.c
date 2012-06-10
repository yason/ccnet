/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "include.h"

#include <glib.h>
#include <stdlib.h>
#include <pthread.h>
#include <event.h>

#include "ccnet-client.h"
#include "processor.h"
#include "proc-factory.h"
#include "timer.h"
#include "peer.h"
#include "utils.h"

G_DEFINE_TYPE (CcnetProcessor, ccnet_processor, G_TYPE_OBJECT);

static void default_shutdown (CcnetProcessor *processor);
static void default_release_resource (CcnetProcessor *processor);

enum {
    DONE_SIG,                   /* connection down */
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

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
    if (!processor->session->connected) {
        g_warning ("[proc] Not connected to daemon.\n");
        return -1;
    }

    processor->failure = PROC_NOTSET;

    return CCNET_PROCESSOR_GET_CLASS (processor)->start (processor, argc, argv);
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
            argv = realloc (argv, sizeof(char *) * max);
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
    
}

static void default_release_resource(CcnetProcessor *processor)
{
    g_free (processor->name);
    g_free (processor->peer_id);
    if (processor->timer)
        ccnet_timer_free (&processor->timer);
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
    CCNET_PROCESSOR_GET_CLASS (processor)->shutdown (processor);

    g_debug ("[proc] Shutdown processor %s(%d) with %d\n", GET_PNAME(processor),
             PRINT_ID(processor->id), reason);

    /* Notify */
    if (!IS_SLAVE (processor)) {
        ccnet_processor_send_update (processor, SC_PROC_DONE, SS_PROC_DONE,
                                     NULL, 0); 
    }

    if (reason == PROC_DONE)
        g_signal_emit (processor, signals[DONE_SIG], 0, TRUE);
    else
        g_signal_emit (processor, signals[DONE_SIG], 0, FALSE);
    ccnet_processor_release_resource (processor);
    ccnet_proc_factory_recycle (processor->session->proc_factory, processor);
}

void
ccnet_processor_done (CcnetProcessor *processor,
                      gboolean success)
{
    if (processor->state == STATE_IN_SHUTDOWN) {
        return;
    }
    processor->state = STATE_IN_SHUTDOWN;
    if (processor->failure == PROC_NOTSET && success)
        processor->failure = PROC_DONE;

    g_debug ("[proc] Processor %s(%d) done\n", GET_PNAME(processor),
             PRINT_ID(processor->id));

    /* Notify */
    if (!IS_SLAVE (processor)) {
        ccnet_processor_send_update (processor, SC_PROC_DONE, SS_PROC_DONE,
                                     NULL, 0); 
    }

    g_signal_emit (processor, signals[DONE_SIG], 0, success);

    ccnet_client_remove_processor (processor->session, processor);
    ccnet_processor_release_resource (processor);
    ccnet_proc_factory_recycle (processor->session->proc_factory, processor);
}

void ccnet_processor_handle_update (CcnetProcessor *processor, 
                                    char *code, char *code_msg,
                                    char *content, int clen)
{
    g_object_ref (processor);
    processor->is_active = TRUE;

    if (code[0] == '5') {
        ccnet_warning ("[Proc] Shutdown processor %s(%d) for bad update: %s %s\n",
                       GET_PNAME(processor), PRINT_ID(processor->id),
                       code, code_msg);

        if (memcmp(code, SC_UNKNOWN_SERVICE, 3) == 0)
            ccnet_processor_shutdown (processor, PROC_NO_SERVICE);
        else if (memcmp(code, SC_PERM_ERR, 3) == 0)
            ccnet_processor_shutdown (processor, PROC_PERM_ERR);
        else
            ccnet_processor_shutdown (processor, PROC_BAD_RESP);
        return;
    }

    if (strncmp (code, SC_PROC_KEEPALIVE, 3) == 0) {
        ccnet_processor_send_response (processor, SC_PROC_ALIVE, 
                                       SS_PROC_ALIVE, NULL, 0);
    } else if (strncmp (code, SC_PROC_DEAD, 3) == 0) {
        g_warning ("[proc] Shutdown processor %s(%d) when peer(%.8s) processor is dead\n",
                   GET_PNAME(processor), PRINT_ID(processor->id),
                   processor->peer_id);
        ccnet_processor_shutdown (processor, PROC_REMOTE_DEAD);
    } else if (strncmp (code, SC_PROC_DONE, 3) == 0) {
        g_debug ("[proc] Shutdown processor when receive 103: service done\n");
        ccnet_processor_done (processor, TRUE);
    } else {
        CCNET_PROCESSOR_GET_CLASS (processor)->handle_update (processor, 
                                                              code, code_msg, 
                                                              content, clen);
    }
    processor->is_active = FALSE;
    g_object_unref (processor);
}

void ccnet_processor_handle_response (CcnetProcessor *processor, 
                                      char *code, char *code_msg,
                                      char *content, int clen)
{
    g_return_if_fail (CCNET_PROCESSOR_GET_CLASS(processor)->handle_response != NULL);

    g_object_ref (processor);
    processor->is_active = TRUE;

    if (code[0] == '5') {
        ccnet_warning ("[Proc] Shutdown processor %s(%d) for bad response: %s %s\n",
                       GET_PNAME(processor), PRINT_ID(processor->id),
                       code, code_msg);

        if (memcmp(code, SC_UNKNOWN_SERVICE, 3) == 0)
            ccnet_processor_shutdown (processor, PROC_NO_SERVICE);
        else if (memcmp(code, SC_PERM_ERR, 3) == 0)
            ccnet_processor_shutdown (processor, PROC_PERM_ERR);
        else
            ccnet_processor_shutdown (processor, PROC_BAD_RESP);
        return;
    }

    if (strncmp (code, SC_PROC_KEEPALIVE, 3) == 0) {
        ccnet_processor_send_update (processor, SC_PROC_ALIVE, 
                                     SS_PROC_ALIVE, NULL, 0);
    } else if (strncmp (code, SC_PROC_DEAD, 3) == 0) {
        g_warning ("[proc] Shutdown processor %s(%d) when peer(%.8s) processor is dead\n",
                   GET_PNAME(processor), PRINT_ID(processor->id),
                   processor->peer_id);
        ccnet_processor_shutdown (processor, PROC_REMOTE_DEAD);
    } else {
        CCNET_PROCESSOR_GET_CLASS (processor)->handle_response (processor, 
                                                                code, code_msg, 
                                                                content, clen);
    }
    processor->is_active = FALSE;
    g_object_unref (processor);
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
    ccnet_client_send_request (processor->session,
                               REQUEST_ID (processor->id),
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

    ccnet_client_send_request (processor->session,
                               REQUEST_ID (processor->id),
                               buf->str);

    g_string_free (buf, TRUE);
}


void
ccnet_processor_send_update(CcnetProcessor *processor, 
                            const char *code,
                            const char *code_msg,
                            const char *content, int clen)
{
    ccnet_client_send_update (processor->session, UPDATE_ID(processor->id), 
                              code, code_msg, content, clen);
}

void
ccnet_processor_send_response(CcnetProcessor *processor, 
                              const char *code,
                              const char *code_msg,
                              const char *content, int clen)
{
    ccnet_client_send_response (processor->session, RESPONSE_ID(processor->id), 
                                code, code_msg, content, clen);
}


/* ------------ thread related functions -------- */

void
ccnet_processor_handle_thread_done (CcnetProcessor *processor,
                                    int status, char *message)
{    
    if (CCNET_PROCESSOR_GET_CLASS (processor)->handle_thread_done) {
        CCNET_PROCESSOR_GET_CLASS (processor)->handle_thread_done(
            processor, status, message
            );
    }
}

static void
notification_cb (int fd, short event, void *arg)
{
    char buf[1024];
    int n;
    CcnetProcessor *processor = CCNET_PROCESSOR(arg);
    char *space, *msg;
    int status;

    n = piperead (processor->pipefd[0], buf, 1024);
    g_assert(n > 0);
    space = strchr (buf, ' ');
    g_assert (space != NULL);
    *(space++) = '\0';
    msg = space;
    status = atoi (buf);

    pipeclose (processor->pipefd[0]);
    pipeclose (processor->pipefd[1]);
    processor->thread_running = FALSE;

    ccnet_processor_handle_thread_done (processor, status, msg);
}

typedef struct {
    CcnetProcessor      *processor;
    CcnetThreadFunc     thread_func;
    CcnetThreadCleanup  thread_cleanup;
} ThreadData;

static void*
ccnet_thread_wrapper (void *vdata)
{
    ThreadData *data = vdata;
    CcnetProcessor *processor;
    CcnetThreadFunc thread_func;
    CcnetThreadCleanup thread_cleanup;
    void *ret;

    /* To avoid memory leak when this thread is canceled, 
     * we free data before calling the thread func.
     */
    processor = data->processor;
    thread_func = data->thread_func;
    thread_cleanup = data->thread_cleanup;
    g_free (data);

    if (thread_cleanup) {
        pthread_cleanup_push (thread_cleanup, processor);
        ret = thread_func (processor);
        /* The cleanup func will always be called when this thread exits. */
        pthread_cleanup_pop (1);
    } else {
        ret = thread_func (processor);
    }

    return ret;
}

int
ccnet_processor_thread_create (CcnetProcessor *processor, 
                               CcnetThreadFunc thread_func,
                               CcnetThreadCleanup cleanup_func)
{
    ThreadData *data;

    if (ccnet_pipe (processor->pipefd) < 0) {
        g_warning ("[proc] pipe error: %s\n", strerror(errno));
        return -1;
    }

    data = g_new (ThreadData, 1);
    data->processor = processor;
    data->thread_func = thread_func;
    data->thread_cleanup = cleanup_func;

    if (pthread_create (&processor->tid, NULL, ccnet_thread_wrapper, data) != 0) {
        g_warning ("[proc] thread creation error: %s\n",
                   strerror(errno));
        return -1;
    }
    processor->thread_running = TRUE;

    event_once (processor->pipefd[0], EV_READ, notification_cb, processor, NULL);

    pthread_detach (processor->tid);

    return 0;
}

int
ccnet_processor_thread_cancel (CcnetProcessor *processor)
{
    if (processor->thread_running)
        return pthread_cancel (processor->tid);
    else
        return 0;
}

void
ccnet_processor_thread_done (CcnetProcessor *processor, int status, char *message)
{
    GString *buf = g_string_new (NULL);

    if (message)
        g_string_append_printf (buf, "%d %s", status, message);
    else
        g_string_append_printf (buf, "%d ", status);

    pipewrite (processor->pipefd[1], buf->str, buf->len+1);

    g_string_free (buf, TRUE);
}

#if 0
static void
thread_pool_func (gpointer data, gpointer vprocessor)
{
    CcnetThreadFunc user_thread_func = data;

    user_thread_func (vprocessor);
}

int
ccnet_processor_thread_pool_new (CcnetProcessor *processor,
                                 int max_threads)
{
    GThreadPool *thread_pool;
    GError      *error;

    thread_pool = g_thread_pool_new (thread_pool_func,
                                     processor,
                                     max_threads,
                                     TRUE,
                                     &error);
    if (error) {
        g_warning ("Failed to create thread pool.\n");
        return -1;
    }

    processor->thread_pool = thread_pool;
    return 0;
}

int
ccnet_processor_thread_pool_run (CcnetProcessor *processor,
                                 CcnetThreadFunc thread_func)
{
    GError *error;

    if (!processor->thread_pool) {
        g_warning ("[proc] Thread pool is not initiated for this processor.\n");
        return -1;
    }

    g_thread_pool_push (processor->thread, thread_func, &error);
    if (error) {
        g_warning ("[proc] Failed to run thread.\n");
        return -1;
    }

    return 0;
}

void
ccnet_processor_thread_pool_free (CcnetProcessor *processor)
{
    g_thread_pool_free (processor->thread_pool, TRUE, FALSE);
}
#endif

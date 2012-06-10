
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ccnetrpc-transport.h"
#include "rpc-common.h"
#include "async-rpc-proc.h"


char *
ccnetrpc_transport_send (void *arg, const gchar *fcall_str,
                       size_t fcall_len, size_t *ret_len)
{
    CcnetrpcTransportParam *priv;
    CcnetClient *session;
    struct CcnetResponse *rsp;
    GString *proc_name;
    uint32_t req_id;

    g_warn_if_fail (arg != NULL && fcall_str != NULL);

    priv = (CcnetrpcTransportParam *)arg;
    session = priv->session;

    req_id = ccnet_client_get_request_id (session);

    proc_name = g_string_new (NULL);
    if (!priv->peer_id)
        g_string_append_printf (proc_name, "%s", priv->service);
    else
        g_string_append_printf (proc_name, "remote %s %s",
                                priv->peer_id, priv->service);
    ccnet_client_send_request (session, req_id, proc_name->str);
    g_string_free (proc_name, TRUE);

    if (ccnet_client_read_response (session) < 0) {
        *ret_len = 0;
        return NULL;
    }
    rsp = &session->response;
    if (memcmp (rsp->code, "200", 3) != 0) {
        g_warning ("[Sea RPC] failed to start rpc server.\n");
        *ret_len = 0;
        return NULL;
    }

    ccnet_client_send_update (session, req_id,
                              SC_CLIENT_CALL, SS_CLIENT_CALL,
                              fcall_str, fcall_len);

    /* read response */
    if (ccnet_client_read_response (session) < 0) {
        *ret_len = 0;
        return NULL;
    }
    rsp = &session->response;

    if (memcmp (rsp->code, SC_SERVER_RET, 3) == 0) {
        *ret_len = (size_t) rsp->clen;
        ccnet_client_send_update (session, req_id,
                                  SC_PROC_DONE, SS_PROC_DONE,
                                  NULL, 0);
        return g_strndup (rsp->content, rsp->clen);
    }

    if (memcmp (rsp->code, SC_SERVER_MORE, 3) == 0) { 
        GString *buf;

        buf = g_string_new_len (rsp->content, rsp->clen);
        *ret_len = (size_t) rsp->clen;

        do {
            ccnet_client_send_update (session, req_id,
                                      SC_CLIENT_MORE, SS_CLIENT_MORE,
                                      fcall_str, fcall_len);
            if (ccnet_client_read_response (session) < 0) {
                *ret_len = 0;
                g_string_free (buf, TRUE);
                return NULL;
            }
            rsp = &session->response;
            if (memcmp (rsp->code, SC_SERVER_ERR, 3) == 0) {
                g_warning ("[Sea RPC] Bad response: %s %s.\n",
                           rsp->code, rsp->code_msg);
                *ret_len = 0;
                g_string_free (buf, TRUE);
                return NULL;
            }

            g_string_append_len (buf, rsp->content, rsp->clen);
            *ret_len += (size_t) rsp->clen;
        } while (memcmp (rsp->code, SC_SERVER_MORE, 3) == 0);

        ccnet_client_send_update (session, req_id,
                                  SC_PROC_DONE, SS_PROC_DONE,
                                  NULL, 0);
        return g_string_free (buf, FALSE);
    }

    g_warning ("[Sea RPC] Bad response: %s %s.\n", rsp->code, rsp->code_msg);
    *ret_len = 0;
    return NULL;
}


int
ccnetrpc_async_transport_send (void *arg, gchar *fcall_str,
                             size_t fcall_len, void *rpc_priv)
{
    CcnetrpcAsyncTransportParam *priv;
    CcnetClient *session;
    CcnetProcessor *proc;

    g_warn_if_fail (arg != NULL && fcall_str != NULL);

    priv = (CcnetrpcAsyncTransportParam *)arg;
    session = priv->session;
    
    if (!priv->peer_id)
        proc = ccnet_proc_factory_create_master_processor (
            session->proc_factory, "async-rpc");
    else
        proc = ccnet_proc_factory_create_remote_master_processor (
            session->proc_factory, "async-rpc", priv->peer_id);
    
    ccnet_async_rpc_proc_set_rpc ((CcnetAsyncRpcProc *)proc, priv->service, 
                                  fcall_str, fcall_len, rpc_priv);
    ccnet_processor_start (proc, 0, NULL);
    return 0;
}

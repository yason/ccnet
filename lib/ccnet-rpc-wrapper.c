/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "include.h"

#include <ccnet.h>
#include <ccnet-object.h>
#include <searpc-client.h>
#include <ccnetrpc-transport.h>

SearpcClient *
ccnet_create_rpc_client (CcnetClient *cclient, const char *peer_id,
                         const char *service_name)
{
    SearpcClient *rpc_client;
    CcnetrpcTransportParam *priv;
    
    priv = g_new0(CcnetrpcTransportParam, 1);
    priv->session = cclient;
    priv->peer_id = g_strdup(peer_id);
    priv->service = g_strdup(service_name);

    rpc_client = searpc_client_new ();
    rpc_client->transport = ccnetrpc_transport_send;
    rpc_client->arg = priv;

    return rpc_client;
}


SEARPC_CLIENT_DEFUN_OBJECT__STRING(get_peer, CCNET_TYPE_PEER);
SEARPC_CLIENT_DEFUN_OBJECT__STRING(get_peer_by_idname, CCNET_TYPE_PEER);
SEARPC_CLIENT_DEFUN_OBJECT__VOID(get_session_info, CCNET_TYPE_SESSION_BASE);
SEARPC_CLIENT_DEFUN_STRING__STRING(get_binding_email);
SEARPC_CLIENT_DEFUN_STRING__STRING(sign_message);
SEARPC_CLIENT_DEFUN_INT__STRING_STRING_STRING(verify_message);
SEARPC_CLIENT_DEFUN_OBJLIST__STRING(get_groups, CCNET_TYPE_GROUP);

SEARPC_CLIENT_ASYNC_DEFUN_OBJECT__STRING(get_peer, CCNET_TYPE_PEER);
SEARPC_CLIENT_ASYNC_DEFUN_STRING__STRING(get_binding_email, 0);

CcnetPeer *
ccnet_get_peer (SearpcClient *client, const char *peer_id)
{
    if (!peer_id)
        return NULL;
    return (CcnetPeer *)get_peer (client, peer_id, NULL);
}

CcnetPeer *
ccnet_get_peer_by_idname (SearpcClient *client, const char *idname)
{
    if (!idname)
        return NULL;
    return (CcnetPeer *)get_peer_by_idname (client, idname, NULL);
}

int
ccnet_get_peer_net_state (SearpcClient *client, const char *peer_id)
{
    CcnetPeer *peer;
    int ret;
    peer = ccnet_get_peer (client, peer_id);
    if (!peer)
        return PEER_DOWN;
    ret = peer->net_state;
    g_object_unref (peer);
    return ret;
}

char *
ccnet_get_default_relay_id (SearpcClient *client)
{
    CcnetSessionBase *base = (CcnetSessionBase *)get_session_info(client, NULL);
    if (base) {
        char *relay_id = NULL;

        if (base->relay_id) {
            relay_id = g_strdup(base->relay_id);
        }

        g_object_unref (base);
        return relay_id;
    }
    return NULL;
}

char *
ccnet_get_binding_email (SearpcClient *client, const char *peer_id)
{
    if (!peer_id)
        return NULL;
    return get_binding_email (client, peer_id, NULL);
}

GList *
ccnet_get_groups_by_user (SearpcClient *client, const char *user)
{
    if (!user)
        return NULL;
    return get_groups (client, user, NULL);
}

int
ccnet_get_peer_async (SearpcClient *client, const char *peer_id,
                      AsyncCallback callback, void *user_data)
{
    return get_peer_async (client, peer_id, callback, user_data);
}

int
ccnet_get_binding_email_async (SearpcClient *client, const char *peer_id,
                               AsyncCallback callback, void *user_data)
{
    return get_binding_email_async (client, peer_id, callback, user_data);
}

char *
ccnet_sign_message (SearpcClient *client, const char *message)
{
    if (!message)
        return NULL;
    return sign_message (client, message, NULL);
}

int
ccnet_verify_message (SearpcClient *client,
                      const char *message,
                      const char *sig_base64,
                      const char *peer_id)
{
    if (!message || !sig_base64 || !peer_id)
        return -1;

    return verify_message (client, message, sig_base64, peer_id, NULL);
}

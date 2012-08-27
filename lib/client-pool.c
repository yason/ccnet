#include "include.h"

#include <ccnet.h>
#include <ccnet/ccnetrpc-transport.h>

#include <glib.h>
#include <pthread.h>

struct CcnetClientPool {
    GQueue *clients;
    pthread_mutex_t lock;
    const char *conf_dir;
};

struct CcnetClientPool *
ccnet_client_pool_new (const char *conf_dir)
{
    CcnetClientPool *pool = g_new0 (CcnetClientPool, 1);

    pool->clients = g_queue_new ();
    pthread_mutex_init (&pool->lock, NULL);
    pool->conf_dir = g_strdup(conf_dir);

    return pool;
}

CcnetClient *
ccnet_client_pool_get_client (struct CcnetClientPool *cpool)
{
    CcnetClient *client;

    pthread_mutex_lock (&cpool->lock);
    client = g_queue_pop_head (cpool->clients);
    pthread_mutex_unlock (&cpool->lock);

    if (!client) {
        client = ccnet_client_new ();
        if (ccnet_client_load_confdir (client, cpool->conf_dir) < 0) {
            g_warning ("[client pool] Failed to load conf dir.\n");
            g_object_unref (client);
            return NULL;
        }
        if (ccnet_client_connect_daemon (client, CCNET_CLIENT_SYNC) < 0) {
            g_warning ("[client pool] Failed to connect.\n");
            g_object_unref (client);
            return NULL;
        }
    }

    return client;
}

void
ccnet_client_pool_return_client (struct CcnetClientPool *cpool,
                                 CcnetClient *client)
{
    pthread_mutex_lock (&cpool->lock);
    g_queue_push_tail (cpool->clients, client);
    pthread_mutex_unlock (&cpool->lock);
}

SearpcClient *
ccnet_client_pool_create_rpc_client (struct CcnetClientPool *cpool,
                                     const char *service)
{
    CcnetClient *client;

    client = ccnet_client_pool_get_client (cpool);
    if (!client)
        return NULL;

    return ccnet_create_rpc_client (client, NULL, service);
}

void
ccnet_client_pool_free_rpc_client (struct CcnetClientPool *cpool,
                                   SearpcClient *rpc_client)
{
    CcnetrpcTransportParam *priv = rpc_client->arg;
    CcnetClient *client = priv->session;

    ccnet_client_pool_return_client (cpool, client);

    ccnet_rpc_client_free (rpc_client);
}

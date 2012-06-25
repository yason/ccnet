/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "common.h"

#include <signal.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>

#include "getgateway.h"
#include "utils.h"

#include "daemon-session.h"
#include "peer.h"
#include "peer-mgr.h"
#include "perm-mgr.h"
#include "packet-io.h"
#include "connect-mgr.h"
#include "message.h"
#include "message-manager.h"
#include "algorithms.h"
#include "net.h"
#include "rsa.h"
#include "ccnet-config.h"

#include "proc-factory.h"


G_DEFINE_TYPE (CcnetDaemonSession, ccnet_daemon_session, CCNET_TYPE_SESSION);

static void set_relay (CcnetSession *session, CcnetPeer *peer);
static int daemon_session_prepare (CcnetSession *session);
static void daemon_session_start (CcnetSession *session);
static void on_peer_auth_done (CcnetSession *session, CcnetPeer *peer);

static void
ccnet_daemon_session_class_init (CcnetDaemonSessionClass *klass)
{
    CcnetSessionClass *session_class = CCNET_SESSION_CLASS (klass);

    session_class->prepare = daemon_session_prepare;
    session_class->start = daemon_session_start;
    session_class->on_peer_auth_done = on_peer_auth_done;
}

static void
ccnet_daemon_session_init (CcnetDaemonSession *session)
{
}

CcnetDaemonSession *
ccnet_daemon_session_new ()
{
    return g_object_new (CCNET_TYPE_DAEMON_SESSION, NULL);
}

int
daemon_session_prepare (CcnetSession *session)
{
    CcnetDaemonSession *daemon_session = (CcnetDaemonSession *)session;
    char *relay_id = NULL;

    relay_id = ccnet_key_file_get_string (session->keyf, "Network", "DEFAULT_RELAY");
    session->base.relay_id = relay_id;

    /* default relay */
    if (session->base.relay_id) {
        /* check relay consistency */
        CcnetPeer *peer = ccnet_peer_manager_get_peer (session->peer_mgr, 
                                                       session->base.relay_id);
        if (peer) {
            if (!ccnet_peer_has_role (peer, "MyRelay")) {
                daemon_session->default_relay = NULL;
                g_object_set (session, "default-relay", "", NULL);
                ccnet_session_save_config (session);
            } else
                set_relay (session, peer);
            g_object_unref (peer);
        }
    }

    return 0;
}

void
daemon_session_start (CcnetSession *session)
{
    
}

static void
on_relay_connected (CcnetPeer *relay, CcnetSession *session)
{
    /* get my dynamic ip and port */
    CcnetPeer *myself = session->myself;
    if (myself->addr_str == NULL) {
        struct sockaddr_in addr;
        socklen_t len = sizeof(struct sockaddr_in);
        int socket = relay->io->socket;
        getsockname (socket, (struct sockaddr *)&addr, &len);
        char *p = inet_ntoa (addr.sin_addr);
        myself->addr_str = strdup (p);
    }
}

static void
connect_relay_signal (CcnetSession *session, CcnetPeer *relay)
{
    g_signal_connect (relay, "auth-done",
                      G_CALLBACK(on_relay_connected), session);
}

static void
disconnect_relay_signal (CcnetSession *session, CcnetPeer *relay)
{
    g_signal_handlers_disconnect_by_func (
        relay, on_relay_connected, session);
}


static void
set_relay (CcnetSession *session, CcnetPeer *peer)
{
    CcnetDaemonSession *daemon_session = (CcnetDaemonSession *)session;

    g_object_set (session, "default-relay", peer->id, NULL);
    daemon_session->default_relay = peer;
    g_object_ref (peer);
    connect_relay_signal (session, peer);
    ccnet_message ("default relay is %s(%.8s)\n", peer->name, peer->id);
}

void
ccnet_daemon_session_set_relay (CcnetDaemonSession *daemon_session,
                                CcnetPeer *peer)
{
    CcnetSession *session = (CcnetSession *)daemon_session;

    if (daemon_session->default_relay && daemon_session->default_relay == peer)
        return;

    if (daemon_session->default_relay) {
        disconnect_relay_signal (session, daemon_session->default_relay);
        ccnet_peer_shutdown (daemon_session->default_relay);
        daemon_session->default_relay = NULL;
    }

    ccnet_peer_manager_add_role (session->peer_mgr, peer, "MyRelay");
    set_relay (session, peer);
    session->saving_pub = 1;
    ccnet_session_save_config (session);
}

/* unset default relay */
void
ccnet_daemon_session_unset_relay (CcnetDaemonSession *daemon_session)
{
    CcnetSession *session = (CcnetSession *)daemon_session;
    if (daemon_session->default_relay) {
        ccnet_peer_manager_remove_role (session->peer_mgr,
                                        daemon_session->default_relay,
                                        "MyRelay");
        disconnect_relay_signal (session, daemon_session->default_relay);
        ccnet_peer_shutdown (daemon_session->default_relay);
        daemon_session->default_relay = NULL;
    }
    g_object_set (session, "default-relay", "", NULL);
    session->saving_pub = 1;
    ccnet_session_save_config (session);
}

static void
on_peer_auth_done (CcnetSession *session, CcnetPeer *peer)
{
    ccnet_peer_manager_send_ready_message (session->peer_mgr, peer);
}

/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef CCNET_SESSION_H
#define CCNET_SESSION_H


#include <event.h>
#include <glib.h>
#include <glib/gstdio.h>
#include "ccnet-session-base.h"

#include "processor.h"
#include "ccnet-db.h"

#include "ccnet-object.h"

#define SESSION_CONFIG_FILENAME   "ccnet.conf"
#define SESSION_PEERDB_NAME       "peer-db"
#define SESSION_RELAYDB_NAME      "relay-db"
#define SESSION_OBJECTDB_NAME     "object-db"
#define SESSION_ID_LENGTH         40


#define CCNET_TYPE_SESSION                  (ccnet_session_get_type ())
#define CCNET_SESSION(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CCNET_TYPE_SESSION, CcnetSession))
#define CCNET_IS_SESSION(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CCNET_TYPE_SESSION))
#define CCNET_SESSION_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CCNET_TYPE_SESSION, CcnetSessionClass))
#define CCNET_IS_SESSION_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CCNET_TYPE_SESSION))
#define CCNET_SESSION_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CCNET_TYPE_SESSION, CcnetSessionClass))


typedef struct CcnetSession CcnetSession;
typedef struct _CcnetSessionClass CcnetSessionClass;

struct _CcnetPeer;


typedef struct _CcnetPreferences CcnetPreferences;


typedef struct _CcnetService {
    char  *svc_name;
    struct _CcnetPeer *provider;
} CcnetService;

#include <openssl/rsa.h>


struct CcnetSession
{
    CcnetSessionBase            base;

    struct _CcnetPeer          *myself;

    const char                 *config_dir;
    const char                 *config_file;
    GKeyFile                   *keyf;

    RSA                        *privkey;
    RSA                        *pubkey;

    struct _CcnetPeerManager   *peer_mgr;

    struct CcnetConnManager    *connMgr;

    struct _CcnetMessageManager *msg_mgr;

    struct _CcnetProcFactory   *proc_factory;

    struct _CcnetPermManager   *perm_mgr;

#ifndef WIN32
    struct event                sigint;
    struct event                sigterm;
    struct event                sigusr1;
#endif

    GHashTable                 *service_hash;

#ifndef CCNET_SERVER
    unsigned int                disable_multicast : 1;
#endif
    unsigned int                saving : 1;
    unsigned int                saving_pub : 1;

    int                         local_port;
    struct event                local_event;

    int                         start_failure;  /* how many times failed 
                                                   to start the network */

    const char                 *objectdb_path;



#ifdef CCNET_SERVER
    struct _CcnetUserManager   *user_mgr;
    struct _CcnetGroupManager  *group_mgr;
    struct _CcnetRelayManager  *relay_mgr; /* used only in relay */
    struct _CcnetClusterManager *cluster_mgr;
    CcnetDB                    *db;

    unsigned int                redirect : 1;
#else
    struct _CcnetPeer          *default_relay;
    /* CcnetDB                    *event_db; */
#endif

    sqlite3                    *config_db;
};

struct _CcnetSessionClass
{
    CcnetSessionBaseClass  parent_class;
};


CcnetSession *ccnet_session_new (const char *config_dir);

void ccnet_session_start (CcnetSession *session);
void ccnet_session_on_exit (CcnetSession *session);
void ccnet_session_save (CcnetSession *session);

int ccnet_session_prepare (CcnetSession *session);

void ccnet_session_save_config (CcnetSession *session);
void ccnet_session_set_relay (CcnetSession *session, struct _CcnetPeer *peer);
void ccnet_session_unset_relay (CcnetSession *session);


void ccnet_session_start_network (CcnetSession *session);
void ccnet_session_shutdown_network (CcnetSession *session);

int ccnet_session_register_service (CcnetSession *session,
                                    const char *svc_name,
                                    const char *group,
                                    struct _CcnetPeer *peer);

CcnetService* ccnet_session_get_service (CcnetSession *session,
                                         const char *service);

void ccnet_session_unregister_service (CcnetSession *session,
                                       struct _CcnetPeer *peer);


#endif

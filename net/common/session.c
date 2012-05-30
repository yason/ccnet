/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "common.h"

#include <signal.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <evdns.h>


#include "getgateway.h"
#include "utils.h"

#include "session.h"
#include "peer.h"
#include "peer-mgr.h"
#include "perm-mgr.h"
#include "packet-io.h"
#include "connect-mgr.h"
#include "message.h"
#include "message-manager.h"
#include "algorithms.h"

#ifdef CCNET_SERVER
#include "user-mgr.h"
#include "group-mgr.h"
#include "relay-mgr.h"
#include "cluster-mgr.h"
#endif

#include "net.h"
#include "rsa.h"
#include "ccnet-config.h"

#include "proc-factory.h"

#include "rpc-service.h"

#define DEBUG_FLAG CCNET_DEBUG_OTHER
#include "log.h"
#define CCNET_EVENT_DB "event.db"
#define CCNET_DB "ccnet.db"

static void ccnet_service_free (CcnetService *service);
#ifndef CCNET_SERVER
static void on_relay_connected (CcnetPeer *relay, CcnetSession *session);
static void connect_relay_signal (CcnetSession *session, CcnetPeer *relay);
static void disconnect_relay_signal (CcnetSession *session, CcnetPeer *relay);
#endif
/* static int _event_db_init(CcnetSession *session); */



G_DEFINE_TYPE (CcnetSession, ccnet_session, CCNET_TYPE_SESSION_BASE);


#ifndef WIN32
static void sigintHandler (int fd, short event, void *vsession)
{
    CcnetSession *session = vsession;
    /* int signo = fd; */

    ccnet_session_on_exit (session);
    exit (1);
}

static void setSigHandlers (CcnetSession *session)
{
    signal (SIGPIPE, SIG_IGN);

    /* event_set(&session->sigchld, SIGCHLD, EV_SIGNAL | EV_PERSIST, */
    /*           sigchldHandler, session); */
	/* event_add(&session->sigchld, NULL); */

    event_set(&session->sigint, SIGINT, EV_SIGNAL,
              sigintHandler, session);
	event_add(&session->sigint, NULL);

    /* same as sigint */
    event_set(&session->sigterm, SIGTERM, EV_SIGNAL,
              sigintHandler, session);
	event_add(&session->sigterm, NULL);

    /* same as sigint */
    event_set(&session->sigusr1, SIGUSR1, EV_SIGNAL,
              sigintHandler, session);
	event_add(&session->sigusr1, NULL);
}
#endif


static void
ccnet_session_class_init (CcnetSessionClass *klass)
{
    /* GObjectClass *gobject_class = G_OBJECT_CLASS (klass); */
}

static void
ccnet_session_init (CcnetSession *session)
{

}


static int load_rsakey(CcnetSession *session)
{
    char *path;
    FILE *fp;
    RSA *key;

    path = g_build_filename(session->config_dir, PEER_KEYFILE, NULL);
    if (!g_file_test(path, G_FILE_TEST_EXISTS))
        ccnet_error ("Can't load rsa private key from %s\n", path);
    if ((fp = g_fopen(path, "rb")) == NULL)
        ccnet_error ("Can't open private key file %s: %s\n", path,
                     strerror(errno));
    if ((key = PEM_read_RSAPrivateKey(fp, NULL, NULL, NULL)) == NULL)
        ccnet_error ("Can't open load key file %s: format error\n", path);
    fclose(fp);

    session->privkey = key;
    session->pubkey = private_key_to_pub(key);
    g_free(path);

    return 0; 
}


#ifdef CCNET_DAEMON
static void
set_relay (CcnetSession *session, CcnetPeer *peer)
{
    g_object_set (session, "default-relay", peer->id, NULL);
    session->default_relay = peer;
    g_object_ref (peer);
    connect_relay_signal (session, peer);
    ccnet_message ("default relay is %s(%.8s)\n", peer->name, peer->id);
}
#endif

static void listen_on_localhost (CcnetSession *session);
static void save_pubinfo (CcnetSession *session);


#ifdef CCNET_SERVER
static int init_sqlite_database (CcnetSession *session)
{
    char *db_path;

    db_path = g_build_filename (session->config_dir, CCNET_DB, NULL);
    session->db = ccnet_db_new_sqlite (db_path);
    if (!session->db) {
        g_warning ("Failed to open database.\n");
        return -1;
    }
    return 0;
}

static int init_mysql_database (CcnetSession *session)
{
    char *host, *user, *passwd, *db, *unix_socket;

    host = ccnet_key_file_get_string (session->keyf, "Database", "HOST");
    user = ccnet_key_file_get_string (session->keyf, "Database", "USER");
    passwd = ccnet_key_file_get_string (session->keyf, "Database", "PASSWD");
    db = ccnet_key_file_get_string (session->keyf, "Database", "DB");

    if (!host) {
        g_warning ("DB host not set in config.\n");
        return -1;
    }
    if (!user) {
        g_warning ("DB user not set in config.\n");
        return -1;
    }
    if (!passwd) {
        g_warning ("DB passwd not set in config.\n");
        return -1;
    }
    if (!db) {
        g_warning ("DB name not set in config.\n");
        return -1;
    }
    unix_socket = ccnet_key_file_get_string (session->keyf,
                                             "Database", "UNIX_SOCKET");
    if (!unix_socket) {
        g_warning ("Unix socket path not set in config.\n");
    }

    session->db = ccnet_db_new_mysql (host, user, passwd, db, unix_socket);
    if (!session->db) {
        g_warning ("Failed to open database.\n");
        return -1;
    }

   return 0;
}
#endif

static int
load_database_config (CcnetSession *session)
{
#ifdef CCNET_SERVER
    int ret;
    char *engine;

    engine = ccnet_key_file_get_string (session->keyf, "Database", "ENGINE");
    if (!engine || strncasecmp (engine, DB_SQLITE, sizeof(DB_SQLITE)) == 0) {
        ccnet_debug ("Use database sqlite\n");
        ret = init_sqlite_database (session);
    } else if (strncasecmp (engine, DB_MYSQL, sizeof(DB_MYSQL)) == 0) {
        ccnet_debug ("Use database Mysql\n");
        ret = init_mysql_database (session);
    }
    return ret;
#else
    return 0;
#endif
}


CcnetSession *
ccnet_session_new (const char *config_dir_r)
{
    char *config_file, *config_dir;
    CcnetSession *session, *ret = NULL;
    char *id = 0, *name = 0, *port_str = 0, *lport_str,
        *user_name = 0, *service_url = 0;
    int port, local_port = 0;
    unsigned char sha1[20];
    GKeyFile *key_file;
#ifdef CCNET_DAEMON
    char *relay_id = 0;
#endif
    config_dir = ccnet_expand_path (config_dir_r);

    if (checkdir(config_dir) < 0) {
        ccnet_error ("Config dir %s does not exist or is not "
                     "a directory.\n", config_dir);
        return NULL;
    }

    config_file = g_build_filename (config_dir, SESSION_CONFIG_FILENAME, NULL);
    key_file = g_key_file_new ();
    g_key_file_set_list_separator (key_file, ',');
    if (!g_key_file_load_from_file (key_file, config_file,
                                    G_KEY_FILE_KEEP_COMMENTS, NULL))
    {
        ccnet_warning ("Can't load config file %s.\n", config_file);
        return NULL;
    }
    
    id = ccnet_key_file_get_string (key_file, "General", "ID");
    user_name = ccnet_key_file_get_string (key_file, "General", "USER_NAME");
    name = ccnet_key_file_get_string (key_file, "General", "NAME");
    service_url = ccnet_key_file_get_string (key_file, "General", "SERVICE_URL");
#ifdef CCNET_DAEMON
    relay_id = ccnet_key_file_get_string (key_file, "Network", "DEFAULT_RELAY");
#endif
    port_str = ccnet_key_file_get_string (key_file, "Network", "PORT");
    lport_str = ccnet_key_file_get_string (key_file, "Client", "PORT");


    if (port_str == NULL)
        port = DEFAULT_PORT;
    else
        port = atoi (port_str);

    if (lport_str != NULL)
        local_port = atoi (lport_str);


    if ( (id == NULL) || (strlen (id) != SESSION_ID_LENGTH) 
         || (hex_to_sha1 (id, sha1) < 0) ) {
        ccnet_error ("Wrong ID\n");
        goto onerror;
    }

    session = g_object_new (CCNET_TYPE_SESSION, NULL);
    memcpy (session->base.id, id, 40);
    session->base.id[40] = '\0';
    session->base.name = g_strdup(name);
    session->base.user_name = g_strdup(user_name);
    session->base.public_port = port;
#ifdef CCNET_DAEMON
    session->base.relay_id = relay_id;
#else
    session->base.relay_id = NULL;
#endif
    if (service_url != NULL)
        session->base.service_url = g_strdup(service_url);
    session->config_file = config_file;
    session->config_dir = config_dir;
    session->local_port = local_port;
    session->keyf = key_file;

    load_rsakey(session);

    if (load_database_config (session) < 0) {
        ccnet_warning ("Failed to load database config.\n");
        goto onerror;
    }
    session->service_hash = g_hash_table_new_full (g_str_hash, g_str_equal, 
                           g_free, (GDestroyNotify)ccnet_service_free);


    /* note, the order is important. */
    session->proc_factory = ccnet_proc_factory_new (session);
    session->peer_mgr = ccnet_peer_manager_new (session);
    session->connMgr = ccnet_conn_manager_new (session);
    session->msg_mgr = ccnet_message_manager_new (session);

#ifdef CCNET_SERVER
    session->user_mgr = ccnet_user_manager_new (session);
    session->group_mgr = ccnet_group_manager_new (session);
    session->cluster_mgr = ccnet_cluster_manager_new (session);
    /* session->relay_mgr = ccnet_relay_manager_new (session); */
#endif

    session->perm_mgr = ccnet_perm_manager_new (session);
    
    ret = session;

onerror:
    g_free (id);
    g_free (name);
    g_free (user_name);
    g_free (port_str);
    return ret;
}

void
ccnet_session_free (CcnetSession *session)
{
    ccnet_peer_manager_free (session->peer_mgr);

    g_object_unref (session);
}


int
ccnet_session_prepare (CcnetSession *session)
{
    char *misc_path;
    /* char *groupdb_path, *reqrdb_path; */

    event_init ();
    evdns_init ();

    session->objectdb_path = g_build_filename (session->config_dir, 
                                               SESSION_OBJECTDB_NAME, NULL);
    if (checkdir_with_mkdir (session->objectdb_path) < 0) {
        ccnet_error ("mkdir %s error", session->objectdb_path);
        return -1;
    }


    misc_path = g_build_filename (session->config_dir, "misc", NULL);
    if (checkdir_with_mkdir (misc_path) < 0) {
        ccnet_error ("mkdir %s error", misc_path);
        return -1;
    }

    /* config db */
    session->config_db = ccnet_session_config_open_db (misc_path);
    if (!session->config_db) {
        ccnet_warning ("Failed to open config db.\n");
        return -1;
    }

    /* _event_db_init(session); */

    /* peer */
    ccnet_peer_manager_load_peerdb (session->peer_mgr);

    /* permission manager */
    ccnet_perm_manager_prepare (session->perm_mgr);

#ifdef CCNET_DAEMON
    /* default relay */
    if (session->base.relay_id) {
        /* check relay consistency */
        CcnetPeer *peer = ccnet_peer_manager_get_peer (session->peer_mgr, 
                                                       session->base.relay_id);
        if (peer) {
            if (!ccnet_peer_has_role (peer, "MyRelay")) {
                session->default_relay = NULL;
                g_object_set (session, "default-relay", "", NULL);
                ccnet_session_save_config (session);
            } else
                set_relay (session, peer);
            g_object_unref (peer);
        }
    }
#endif

#ifdef CCNET_SERVER
    ccnet_user_manager_prepare (session->user_mgr);
    /* if (session->is_relay) { */
    /*     char *relaydb_path = g_build_filename (session->config_dir,  */
    /*                                            SESSION_RELAYDB_NAME, NULL); */
    /*     ccnet_relay_manager_load_db (session->relay_mgr, relaydb_path); */
    /* } */
#endif

    /* Open localhost, if failed, then the program will exists. This is used
     * to prevent two instance of ccnet on the same port.
     */
    listen_on_localhost (session);

    /* refresh pubinfo on every startup */
    save_pubinfo (session);

    g_free (misc_path);
    return 0;
}

static void
save_peerinfo (CcnetSession *session)
{
    FILE *fp;
    char *path;
    char filename[64];
    
    sprintf(filename, "%s%s", session->base.id, ".peer");
    path = g_build_filename (session->config_dir, "misc", filename, NULL);

    if ((fp = g_fopen(path, "wb")) == NULL) {
        ccnet_warning ("Open public info file %s error: %s\n",
                       path, strerror(errno));
        g_free (path);
        return;
    }
    g_free (path);

    ccnet_message ("Update pubinfo file\n");
    session->myself->public_port = session->base.public_port;

    GString *str = ccnet_peer_to_string (session->myself);
    fputs (str->str, fp);
    fclose (fp);
    g_string_free (str, TRUE);
}


static void save_pubinfo (CcnetSession *session)
{
    save_peerinfo(session);
}

void
ccnet_session_save_config (CcnetSession *session)
{
    GError *error = NULL;
    char *str;
    FILE *fp;

    ccnet_message ("[Session] Saving configure file\n");
    if (session->saving_pub) {
        /* only update timestamp when pubinfo changes */
        save_pubinfo (session);
        session->saving_pub = 0;
    }

    g_key_file_set_string (session->keyf, "General", "NAME",
                           session->base.name);
    g_key_file_set_string (session->keyf, "General", "ID", 
                           session->base.id);
    g_key_file_set_string (session->keyf, "General", "USER_NAME", 
                           session->base.user_name);
#ifdef CCNET_SERVER
    g_key_file_set_string (session->keyf, "General", "SERVICE_URL",
                           session->base.service_url?session->base.service_url:"");
#endif
#ifdef CCNET_DAEMON
    g_key_file_set_string (session->keyf, "Network", "DEFAULT_RELAY",
                session->base.relay_id ? session->base.relay_id : "");
#endif
    g_key_file_set_integer (session->keyf, "Network", "PORT",
                            session->base.public_port);

    g_key_file_set_integer (session->keyf, "Client", "PORT", session->local_port);

    str = g_key_file_to_data (session->keyf, NULL, &error);
    if (error) {
        ccnet_warning ("Can't save unauth peer info: %s\n",
                       error->message);
        return;
    }

    if ((fp = g_fopen (session->config_file, "wb")) == NULL) {
        ccnet_warning ("Can't save session conf: %s\n", strerror(errno));
        g_free (str);
        return;
    }

    fputs (str, fp);
    fclose (fp);

    g_free (str);


    return;
}

void
ccnet_session_save (CcnetSession *session)
{
    /* ccnet_req_manager_backup_requirements (session->reqMgr); */
}

void
ccnet_session_on_exit (CcnetSession *session)
{
    time_t t;

    ccnet_peer_manager_on_exit (session->peer_mgr);
    ccnet_session_save (session);

    t = time(NULL);
    ccnet_message ("Exit at %s\n", ctime(&t));
    ccnet_session_free (session);
}

#ifdef CCNET_DAEMON
void
ccnet_session_set_relay (CcnetSession *session, CcnetPeer *peer)
{
    if (session->default_relay && session->default_relay == peer)
        return;

    if (session->default_relay) {
        disconnect_relay_signal (session, session->default_relay);
        session->default_relay = NULL;
    }

    ccnet_peer_manager_add_role (session->peer_mgr, peer, "MyRelay");
    set_relay (session, peer);
    session->saving_pub = 1;
    ccnet_session_save_config (session);
}

/* unset default relay */
void
ccnet_session_unset_relay (CcnetSession *session)
{
    if (session->default_relay) {
        ccnet_peer_manager_remove_role (session->peer_mgr, session->default_relay,
                                        "MyRelay");
        disconnect_relay_signal (session, session->default_relay);
        session->default_relay = NULL;
    }
    g_object_set (session, "default-relay", "", NULL);
    session->saving_pub = 1;
    ccnet_session_save_config (session);
}
#endif

static const char *net_status_string (int status)
{
    switch (status) {
    case NET_STATUS_DOWN:
        return "Down";
    case NET_STATUS_INNAT:
        return "In nat";
    case NET_STATUS_FULL:
        return "Full";
    default:
        return "Unknown";
    }
}

static void accept_local_client (int fd, short event, void *vsession)
{
    CcnetSession *session = vsession;
    CcnetPacketIO *io;
    int connfd;
    CcnetPeer *peer;
    static int local_id = 0;

    connfd = accept (fd, NULL, 0);

    ccnet_message ("Accepted a local client\n");

    io = ccnet_packet_io_new_incoming (session, NULL, connfd);
    peer = ccnet_peer_new (session->base.id);
    peer->name = g_strdup_printf("local-%d", local_id++);
    peer->is_local = TRUE;
    ccnet_peer_set_io (peer, io);
    ccnet_peer_set_net_state (peer, PEER_CONNECTED);
    ccnet_peer_manager_add_local_peer (session->peer_mgr, peer);
    g_object_unref (peer);
}

static void listen_on_localhost (CcnetSession *session)
{
    int sockfd;

    if ( (sockfd = ccnet_net_bind_v4 ("127.0.0.1", &session->local_port)) < 0) {
        printf ("listen on localhost failed\n");
        exit (1);
    } ccnet_message ("Listen on 127.0.0.1 %d\n", session->local_port);

    listen (sockfd, 5);
    event_set (&session->local_event, sockfd, EV_READ | EV_PERSIST, 
               accept_local_client, session);
    event_add (&session->local_event, NULL);
}

void
ccnet_session_start_network (CcnetSession *session)
{
    int r;

    r = in_nat ();
    if (r == -1)
        session->base.net_status = NET_STATUS_DOWN;
    else if (r == 0)
        session->base.net_status = NET_STATUS_FULL;
    else
        session->base.net_status = NET_STATUS_INNAT;
    ccnet_message ("Net status: %s\n", net_status_string(session->base.net_status));

    if (session->base.net_status != NET_STATUS_DOWN) {
        ccnet_conn_manager_start (session->connMgr);

        session->myself->in_nat = r;
        session->myself->addr_str = NULL;
    }
}

void
ccnet_session_shutdown_network (CcnetSession *session)
{
    GList *peers, *ptr;
    
    peers = ccnet_peer_manager_get_peer_list(session->peer_mgr);
    for (ptr = peers; ptr; ptr = ptr->next)
        ccnet_peer_shutdown ((CcnetPeer *)ptr->data);
    g_list_free (peers);

    ccnet_conn_manager_stop (session->connMgr);
}

static int
restart_network (CcnetSession *session)
{
    g_assert (session->base.net_status == NET_STATUS_DOWN);

    ccnet_session_start_network (session);
    if (session->base.net_status != NET_STATUS_DOWN) {
        session->start_failure = 0;
        return FALSE;
    }
    
    if (++session->start_failure > 3) 
        return FALSE;

    return TRUE;
}

void
ccnet_session_start (CcnetSession *session)
{
    ccnet_proc_factory_start (session->proc_factory);
    ccnet_message_manager_start (session->msg_mgr);

#ifndef WIN32
    setSigHandlers (session);
#endif

    ccnet_session_start_network (session);
    if (session->base.net_status == NET_STATUS_DOWN) {
        ccnet_timer_new ((TimerCB)restart_network, session, 10000);
    }

    ccnet_peer_manager_start (session->peer_mgr);
#ifdef CCNET_SERVER
    ccnet_cluster_manager_start (session->cluster_mgr);
#endif
    ccnet_start_rpc(session);

    /* event_set_log_callback (logFunc); */
    event_dispatch ();
}



static void
ccnet_service_free (CcnetService *service)
{
    g_free (service->svc_name);
    g_free (service);
}

int
ccnet_session_register_service (CcnetSession *session,
                                const char *svc_name,
                                const char *group,
                                CcnetPeer *peer)
{
    CcnetService *service = g_new0 (CcnetService, 1);

    g_assert (peer->is_local);
    g_assert (group);

    if (g_hash_table_lookup (session->service_hash, svc_name)) {
        ccnet_debug ("[Service] Service %s has already been registered\n",
                     svc_name);
        return -1;
    }
    ccnet_debug ("[Service] Service %s registered\n", svc_name);

    ccnet_perm_manager_register_service (session->perm_mgr, svc_name,
                                         group, peer);

    service->svc_name = g_strdup(svc_name);
    service->provider = peer;

    g_hash_table_insert (session->service_hash, g_strdup(svc_name), service);

    return 0;
}

CcnetService*
ccnet_session_get_service (CcnetSession *session,
                           const char *svc_name)
{
    return g_hash_table_lookup (session->service_hash, svc_name);
}

gboolean remove_service_cmp (gpointer key,
                             gpointer value,
                             gpointer user_data)
{
    CcnetService *service = value;

    if (service->provider == user_data) {
        ccnet_debug ("[Service] Service %s un-registered\n", (char *)key);
        return TRUE;
    }
    return FALSE;
}

void
ccnet_session_unregister_service (CcnetSession *session,
                                  CcnetPeer *peer)
{
    g_hash_table_foreach_remove (session->service_hash, 
                                 remove_service_cmp, peer);
}

#ifdef CCNET_DAEMON
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
    /* for reinitiate */
    ccnet_peer_shutdown (relay);
}

static void
on_relay_connected (CcnetPeer *relay, CcnetSession *session)
{
    /* CcnetProcessor *proc; */

    /* Synchronize my authentication information to my relay */
    /* proc = ccnet_proc_factory_create_master_processor (session->proc_factory, */
    /*                                                    "sync-relay", relay); */
    /* ccnet_processor_startl (proc, NULL); */

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
#endif


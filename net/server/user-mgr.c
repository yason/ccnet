/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "common.h"

#include <sys/stat.h>
#include <dirent.h>

#include "ccnet-db.h"
#include "timer.h"
#include "utils.h"


#include "peer.h"
#include "session.h"
#include "peer-mgr.h"
#include "user-mgr.h"

#include <openssl/sha.h>

#define DEBUG_FLAG  CCNET_DEBUG_PEER
#include "log.h"

#define DEFAULT_SAVING_INTERVAL_MSEC 30000


G_DEFINE_TYPE (CcnetUserManager, ccnet_user_manager, G_TYPE_OBJECT);


#define GET_PRIV(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), CCNET_TYPE_USER_MANAGER, CcnetUserManagerPriv))


static int open_db (CcnetUserManager *manager);

struct CcnetUserManagerPriv {
    CcnetDB    *db;
};


static void
ccnet_user_manager_class_init (CcnetUserManagerClass *klass)
{

    g_type_class_add_private (klass, sizeof (CcnetUserManagerPriv));
}

static void
ccnet_user_manager_init (CcnetUserManager *manager)
{
    manager->priv = GET_PRIV(manager);
}

CcnetUserManager*
ccnet_user_manager_new (CcnetSession *session)
{
    CcnetUserManager* manager;

    manager = g_object_new (CCNET_TYPE_USER_MANAGER, NULL);
    manager->session = session;
    manager->user_hash = g_hash_table_new (g_str_hash, g_str_equal);
    manager->userdb_path = g_build_filename (session->config_dir, "user-db", NULL);

    return manager;
}

int
ccnet_user_manager_prepare (CcnetUserManager *manager)
{
    int ret;
    if ( (ret = open_db(manager)) < 0)
        return ret;

    return 0;
}

void
ccnet_user_manager_free (CcnetUserManager *manager)
{
    g_object_unref (manager);
}

void
ccnet_user_manager_start (CcnetUserManager *manager)
{

}

void ccnet_user_manager_on_exit (CcnetUserManager *manager)
{
}


/* -------- DB Operations -------- */

static void check_db_table (CcnetDB *db)
{
    char *sql;

    int db_type = ccnet_db_type (db);
    if (db_type == CCNET_DB_TYPE_MYSQL) {
        sql = "CREATE TABLE IF NOT EXISTS EmailUser ("
            "id INTEGER NOT NULL PRIMARY KEY AUTO_INCREMENT, "
            "email VARCHAR(255), passwd CHAR(41), "
            "is_staff BOOL NOT NULL, is_active BOOL NOT NULL, "
            "ctime INTEGER, INDEX (EMAIL(20)))";
        ccnet_db_query (db, sql);
        sql = "CREATE TABLE IF NOT EXISTS Binding (email VARCHAR(255), peer_id CHAR(41),"
            "UNIQUE INDEX (peer_id), INDEX (email(20)))";
        ccnet_db_query (db, sql);
    } else if (db_type == CCNET_DB_TYPE_SQLITE) {
        sql = "CREATE TABLE IF NOT EXISTS EmailUser (id INTEGER NOT NULL PRIMARY KEY,"
            " email TEXT, passwd TEXT, is_staff bool NOT NULL, is_active bool NOT NULL,"
            " ctime INTEGER)";
        ccnet_db_query (db, sql);
        sql = "CREATE INDEX IF NOT EXISTS email_index on EmailUser (email)";
        ccnet_db_query (db, sql);

        sql = "CREATE TABLE IF NOT EXISTS Binding (email TEXT, peer_id TEXT)";
        ccnet_db_query (db, sql);
        sql = "CREATE INDEX IF NOT EXISTS email_index on Binding (email)";
        ccnet_db_query (db, sql);
        sql = "CREATE UNIQUE INDEX IF NOT EXISTS peer_index on Binding (peer_id)";
        ccnet_db_query (db, sql);
    }
}


static CcnetDB *
open_sqlite_db (CcnetUserManager *manager)
{
    CcnetDB *db = NULL;
    char *db_dir;
    char *db_path;

    db_dir = g_build_filename (manager->session->config_dir, "PeerMgr", NULL);
    if (checkdir_with_mkdir(db_dir) < 0) {
        ccnet_error ("Cannot open db dir %s: %s\n", db_dir,
                     strerror(errno));
        return NULL;
    }
    g_free (db_dir);

    db_path = g_build_filename (manager->session->config_dir, "PeerMgr",
                                "usermgr.db", NULL);
    db = ccnet_db_new_sqlite (db_path);
    g_free (db_path);

    return db;
}

static int
open_db (CcnetUserManager *manager)
{
    CcnetDB *db;

    switch (ccnet_db_type(manager->session->db)) {
    /* To be compatible with the db file layout of 0.9.1 version,
     * we don't use conf-dir/ccnet.db for user and peer info, but
     * user conf-dir/PeerMgr/peermgr.db and conf-dir/PeerMgr/usermgr.db instead.
     */
    case CCNET_DB_TYPE_SQLITE:
        db = open_sqlite_db (manager);
        if (!db)
            return -1;
        break;
    case CCNET_DB_TYPE_MYSQL:
        db = manager->session->db;
        break;
    }

    manager->priv->db = db;
    check_db_table (db);
    return 0;
}


/* -------- EmailUser Management -------- */

int
ccnet_user_manager_add_emailuser (CcnetUserManager *manager, const char *email,
                                  const char *passwd, int is_staff, int is_active)
{
    CcnetDB *db = manager->priv->db;
    gint64 now = get_current_time();
    char sql[512];
    unsigned char sha1[20];
    SHA_CTX s;
    char encry_passwd[41];

    SHA1_Init (&s);
    SHA1_Update (&s, passwd, strlen(passwd));
    SHA1_Final (sha1, &s);

    rawdata_to_hex (sha1, encry_passwd, 20);
    
    snprintf (sql, 512, "INSERT INTO EmailUser(email, passwd, is_staff, is_active, "
              "ctime) VALUES ('%s', '%s', '%d', '%d', "
              "%"G_GINT64_FORMAT")", email, encry_passwd, is_staff, is_active,
              now);
    
    return ccnet_db_query (db, sql);
}

int
ccnet_user_manager_remove_emailuser (CcnetUserManager *manager, const char *email)
{
    CcnetDB *db = manager->priv->db;
    char sql[512];

    snprintf (sql, 512, "DELETE FROM EmailUser WHERE email='%s'", email);

    return ccnet_db_query (db, sql);
}

int
ccnet_user_manager_validate_emailuser (CcnetUserManager *manager, const char *email,
                                       const char *passwd)
{
    CcnetDB *db = manager->priv->db;
    char sql[512];
    unsigned char sha1[20];
    SHA_CTX s;
    char encry_passwd[41];

    SHA1_Init (&s);
    SHA1_Update (&s, passwd, strlen(passwd));
    SHA1_Final (sha1, &s);
    rawdata_to_hex (sha1, encry_passwd, 20);
    
    snprintf (sql, 512, "SELECT email FROM EmailUser WHERE email='%s' AND "
              "passwd='%s'", email, encry_passwd);
    
    return ccnet_db_check_for_existence (db, sql);
}

static gboolean
get_emailuser_cb (CcnetDBRow *row, void *data)
{
    CcnetEmailUser **p_emailuser = data;

    int id = ccnet_db_row_get_column_int (row, 0);
    const char *email = (const char *)ccnet_db_row_get_column_text (row, 1);
    const char* passwd = (const char *)ccnet_db_row_get_column_text (row, 2);
    int is_staff = ccnet_db_row_get_column_int (row, 3);
    int is_active = ccnet_db_row_get_column_int (row, 4);
    gint64 ctime = ccnet_db_row_get_column_int64 (row, 5);
    *p_emailuser = g_object_new (CCNET_TYPE_EMAIL_USER,
                              "id", id,
                              "email", email,
                              "passwd", passwd,
                              "is_staff", is_staff,
                              "is_active", is_active,
                              "ctime", ctime,
                              NULL);

    return FALSE;
}

CcnetEmailUser*
ccnet_user_manager_get_emailuser (CcnetUserManager *manager,
                                  const char *email)
{
    CcnetDB *db = manager->priv->db;
    char sql[512];
    CcnetEmailUser *emailuser = NULL;

    snprintf (sql, sizeof(sql), 
              "SELECT id, email, passwd, is_staff, is_active, ctime"
              " FROM EmailUser WHERE email='%s'", email);
    if (ccnet_db_foreach_selected_row (db, sql, get_emailuser_cb, &emailuser) < 0)
        return NULL;
    
    return emailuser;
}

CcnetEmailUser*
ccnet_user_manager_get_emailuser_by_id (CcnetUserManager *manager, int id)
{
    CcnetDB *db = manager->priv->db;
    char sql[512];
    CcnetEmailUser *emailuser = NULL;

    snprintf (sql, sizeof(sql), 
              "SELECT id, email, passwd, is_staff, is_active, ctime"
              " FROM EmailUser WHERE id='%d'", id);
    if (ccnet_db_foreach_selected_row (db, sql, get_emailuser_cb, &emailuser) < 0)
        return NULL;
    
    return emailuser;
}

static gboolean
get_emailusers_cb (CcnetDBRow *row, void *data)
{
    GList **plist = data;
    CcnetEmailUser *emailuser;

    int id = ccnet_db_row_get_column_int (row, 0);
    const char *email = (const char *)ccnet_db_row_get_column_text (row, 1);
    const char* passwd = (const char *)ccnet_db_row_get_column_text (row, 2);
    int is_staff = ccnet_db_row_get_column_int (row, 3);
    int is_active = ccnet_db_row_get_column_int (row, 4);
    gint64 ctime = ccnet_db_row_get_column_int64 (row, 5);
    emailuser = g_object_new (CCNET_TYPE_EMAIL_USER,
                              "id", id,
                              "email", email,
                              "passwd", passwd,
                              "is_staff", is_staff,
                              "is_active", is_active,
                              "ctime", ctime,
                              NULL);

    *plist = g_list_prepend (*plist, emailuser);

    return TRUE;
}

GList*
ccnet_user_manager_get_emailusers (CcnetUserManager *manager, int start, int limit)
{
    CcnetDB *db = manager->priv->db;
    GList *ret = NULL;
    char sql[256];

    if (start == -1 && limit == -1)
        snprintf (sql, 256, "SELECT * FROM EmailUser");
    else
        snprintf (sql, 256, "SELECT * FROM EmailUser LIMIT %d, %d", start, limit);

    if (ccnet_db_foreach_selected_row (db, sql, get_emailusers_cb, &ret) < 0) {
        while (ret != NULL) {
            g_object_unref (ret->data);
            ret = g_list_delete_link (ret, ret);
        }
        return NULL;
    }

    return g_list_reverse (ret);
}

int
ccnet_user_manager_update_emailuser (CcnetUserManager *manager, int id,
                                     const char* passwd, int is_staff, int is_active)
{
    CcnetDB* db = manager->priv->db;
    char sql[512];

    snprintf (sql, 512, "UPDATE EmailUser SET passwd='%s', is_staff='%d', "
              "is_active='%d' WHERE id='%d'", passwd, is_staff, is_active, id);

    return ccnet_db_query (db, sql);
}


int
ccnet_user_manager_add_binding (CcnetUserManager *manager, const char *email,
                                const char *peer_id)
{
    CcnetDB *db = manager->priv->db;
    char *binding_email = NULL;
    char sql[512];
    int ret;

    /* since one email can bind many peer ids, but one peer id can only be */
    /*binded by one email, when we add email peerid binding: */
    /* we check whether the peer id has been binded, if not, just add binding;*/
    /* if already binded: if old email and new email are same, return success;*/
    /* else reutrn error */
    binding_email = ccnet_user_manager_get_binding_email (manager, peer_id);
    if (binding_email == NULL) {
        snprintf (sql, 512, "INSERT INTO Binding(email, peer_id) VALUES "
                  "('%s', '%s')", email, peer_id);
        ret = ccnet_db_query (db, sql);
    } else {
        if (g_strcmp0 (binding_email, email) == 0) {
            ret = 0;	// the peer id already binded by this email
        } else {
            ret = -2;	// the peer id is binded by other email
        }
    }

    /* notify client the peer id is binded */
    if (ret == 0) {
        ccnet_peer_manager_send_bind_status (manager->session->peer_mgr,
                                             peer_id, email);
    }

    g_free (binding_email);
    return ret;
}

int
ccnet_user_manager_remove_one_binding (CcnetUserManager *manager,
                                       const char *email,
                                       const char *peer_id)
{
    CcnetDB *db = manager->priv->db;
    char sql[512];

    snprintf (sql, 512,
              "DELETE FROM Binding WHERE email='%s' and peer_id = '%s'",
              email, peer_id);

    /* notify client the peer id is not binded */
    ccnet_peer_manager_send_bind_status (manager->session->peer_mgr, peer_id,
                                         "not-bind");

    return ccnet_db_query (db, sql);
}

int
ccnet_user_manager_remove_binding (CcnetUserManager *manager, const char *email)
{
    CcnetDB *db = manager->priv->db;
    char sql[512];

    snprintf (sql, 512, "DELETE FROM Binding WHERE email='%s'", email);

    return ccnet_db_query (db, sql);
}

char *
ccnet_user_manager_get_binding_email (CcnetUserManager *manager, const char *peer_id)
{
    char sql[512];

    snprintf (sql, 512, "SELECT email FROM Binding WHERE peer_id='%s'", peer_id);
    return ccnet_db_get_string (manager->priv->db, sql);
}

static gboolean
get_binding_peerids (CcnetDBRow *row, void *data)
{
    GList **list = (GList **)data;
    const char *peer_id = (char *) ccnet_db_row_get_column_text (row, 0);

    *list = g_list_prepend (*list, g_strdup (peer_id));
    return TRUE;
}

GList *
ccnet_user_manager_get_binding_peerids (CcnetUserManager *manager, const char *email)
{
    char sql[512];
    GList *ret = NULL;

    snprintf (sql, 512, "SELECT peer_id FROM Binding WHERE email='%s'", email);
    ccnet_db_foreach_selected_row (manager->priv->db, sql,
                                   get_binding_peerids, &ret);
    return ret;
}



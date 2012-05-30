/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "common.h"

#include "ccnet-db.h"
#include "utils.h"
#include "timer.h"

#include "peer.h"
#include "user.h"
#include "session.h"

#include "relay-mgr.h"

#define DEBUG_FLAG CCNET_DEBUG_REQUIREMENT
#include "log.h"

#define CCNET_RELAY_DB "relay.db"
#define USER_PRUNE_PERIOD_SEC (3600*24) /* one day */
#define USER_PRUNE_THRESHOD_SEC (3600*24*7) /* one week */

struct RelayManagerPriv {
    CcnetDB *relay_db;

    /* struct CcnetTimer *gc_timer; */
};

#define GET_PRIV(o)  \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), CCNET_TYPE_RELAY_MANAGER, RelayManagerPriv))

G_DEFINE_TYPE (CcnetRelayManager, ccnet_relay_manager, G_TYPE_OBJECT);


static void
ccnet_relay_manager_class_init (CcnetRelayManagerClass *class)
{
    g_type_class_add_private (class, sizeof(RelayManagerPriv));
}

static void
ccnet_relay_manager_init (CcnetRelayManager *manager)
{
    manager->priv = GET_PRIV(manager);
}

CcnetRelayManager *
ccnet_relay_manager_new (CcnetSession *session)
{
    CcnetRelayManager *manager;

    manager = g_object_new (CCNET_TYPE_RELAY_MANAGER, NULL);
    manager->session = session;

    return manager;
}

static void sqlite_check_db_table (CcnetDB *db)
{
    char *sql;

    sql = "CREATE TABLE IF NOT EXISTS permission ("
        "relay_dst_id TEXT, relay_src_id TEXT);";
    ccnet_db_query (db, sql);

    sql = "CREATE INDEX IF NOT EXISTS relay_dst_index ON permission (relay_dst_id);";
    ccnet_db_query (db, sql);

    sql = "CREATE TABLE IF NOT EXISTS groups ("
        "peer_id TEXT, group_id TEXT);";
    ccnet_db_query (db, sql);

    sql = "CREATE INDEX IF NOT EXISTS groups_index ON groups (peer_id);";
    ccnet_db_query (db, sql);
    
    sql = "CREATE TABLE IF NOT EXISTS RelayUserRole ("
        "user_id1 TEXT, user_id2 TEXT, roles TEXT, timestamp INTEGER)";
    ccnet_db_query (db, sql);

    sql = "CREATE INDEX IF NOT EXISTS user_role_index ON "
        "RelayUserRole (user_id1)";
    ccnet_db_query (db, sql);

    sql = "CREATE TABLE IF NOT EXISTS RelayPruneTime (next_time TEXT);";
    ccnet_db_query (db, sql);

    sql = "CREATE TABLE IF NOT EXISTS UserProfile (user_id TEXT, profile TEXT, "
        " timestamp INTEGER);";
    ccnet_db_query (db, sql);

    sql = "CREATE INDEX IF NOT EXISTS user_profile_index ON "
        "UserProfile (user_id)";
    ccnet_db_query (db, sql);
}

#ifdef MULT_DATABASE
static void mysql_check_db_table (CcnetDB *db)
{
    char *sql;

    sql = "CREATE TABLE IF NOT EXISTS permission ("
        "relay_dst_id CHAR(41), relay_src_id CHAR(41), INDEX (relay_dst_id));";
    ccnet_db_query (db, sql);

    sql = "CREATE TABLE IF NOT EXISTS groups ("
        "peer_id CHAR(41), group_id TEXT, INDEX (peer_id));";
    ccnet_db_query (db, sql);

    sql = "CREATE TABLE IF NOT EXISTS RelayUserRole (user_id1 CHAR(41), "
        "user_id2 CHAR(41), roles TEXT, timestamp INTEGER, INDEX (user_id1))";
    ccnet_db_query (db, sql);

    sql = "CREATE TABLE IF NOT EXISTS RelayPruneTime (next_time TEXT);";
    ccnet_db_query (db, sql);

    sql = "CREATE TABLE IF NOT EXISTS UserProfile (user_id CHAR(41), profile TEXT, "
        " timestamp INTEGER, INDEX(user_id));";
    ccnet_db_query (db, sql);
}
#endif

int
ccnet_relay_manager_load_db (CcnetRelayManager *manager,
                             const char *relay_db)
{
    CcnetDB *db;
#ifndef MULT_DATABASE
    char *db_path;

    if (checkdir_with_mkdir(relay_db) < 0) {
        ccnet_warning ("Cannot open relay dir %s: %s\n", relay_db,
                       strerror(errno));
        return -1;
    }

    db_path = g_strconcat (relay_db, "/", CCNET_RELAY_DB, NULL);
    if (sqlite_open_db (db_path, &db) < 0) {
        g_free (db_path);
        return -1;
    }
    g_free (db_path);

    manager->priv->relay_db = db;
    sqlite_check_db_table (db);
#else
    db = manager->session->db;
    manager->priv->relay_db = db;

    int db_type = ccnet_db_type (db);
    if (db_type == CCNET_DB_TYPE_SQLITE) {
        sqlite_check_db_table (db);
    } else if (db_type == CCNET_DB_TYPE_MYSQL) {
        mysql_check_db_table (db);
    }
#endif
    return 0;
}

void
ccnet_relay_manager_start (CcnetRelayManager *manager)
{

}

void
ccnet_relay_manager_add_permission (CcnetRelayManager *manager,
                                    const char* relay_dst_id,
                                    const char* relay_src_id)
{
    RelayManagerPriv *priv = manager->priv;
    char sql[512];

    if (!relay_src_id || !relay_dst_id)
        return;

    snprintf (sql, 512, "INSERT INTO permission VALUES ('%s', '%s');",
              relay_dst_id, relay_src_id);
    ccnet_db_query (priv->relay_db, sql);
}

void
ccnet_relay_manager_del_permission (CcnetRelayManager *manager,
                                    const char* relay_dst_id,
                                    const char* relay_src_id)
{
    RelayManagerPriv *priv = manager->priv;
    char sql[512];

    if (!relay_src_id || !relay_dst_id)
        return;

    snprintf (sql, 512, "DELETE FROM permission WHERE "
              "relay_dst_id = '%s' AND relay_src_id = '%s';",
              relay_dst_id, relay_src_id);
    ccnet_db_query (priv->relay_db, sql);
}

gboolean
ccnet_relay_manager_check_permission (CcnetRelayManager *manager,
                                      const char* relay_dst_id,
                                      const char* relay_src_id)
{
    RelayManagerPriv *priv = manager->priv;
    char sql[1024];

    if (!relay_src_id || !relay_dst_id)
        return FALSE;

    snprintf (sql, 1024, "SELECT * FROM permission WHERE "
              "relay_dst_id = '%s' AND relay_src_id = '%s';",
              relay_dst_id, relay_src_id);

    return ccnet_db_check_for_existence (priv->relay_db, sql);
}

enum {
    RELAY_DB_COLUMN_DST_ID = 0,
    RELAY_DB_COLUMN_SRC_ID,
};

static gboolean get_peer_cb (CcnetDBRow *row, void *data)
{
    GList **peers = (GList **)data;
    const char *peer_id = (char *) ccnet_db_row_get_column_text(
        row, RELAY_DB_COLUMN_SRC_ID);
    *peers = g_list_prepend (*peers, g_strdup(peer_id));
    return TRUE;
}

GList*
ccnet_relay_manager_get_peer_list (CcnetRelayManager *manager,
                                   const char* relay_dst_id)
{
    RelayManagerPriv *priv = manager->priv;
    char sql[1024];
    int result;
    GList *peers = NULL;

    if (!relay_dst_id)
        return NULL;

    snprintf (sql, 1024, "SELECT * FROM permission WHERE relay_dst_id = '%s';",
              relay_dst_id);
    result = ccnet_db_foreach_selected_row (priv->relay_db, sql,
                                            get_peer_cb, &peers);

    if (result < 0) {
        if (peers) {
            g_list_free (peers);
            peers = NULL;
        }
    }

    return peers;
}

void ccnet_relay_manager_add_group (CcnetRelayManager *manager,
                                    const char *peer_id,
                                    const char *group_id)
{
    RelayManagerPriv *priv = manager->priv;
    char sql[512];

    if (!peer_id || !group_id)
        return;

    snprintf (sql, 512, "INSERT INTO groups VALUES ('%s', '%s');",
              peer_id, group_id);
    ccnet_db_query (priv->relay_db, sql);
}

void
ccnet_relay_manager_del_group (CcnetRelayManager *manager,
                               const char* peer_id,
                               const char* group_id)
{
    RelayManagerPriv *priv = manager->priv;
    char sql[512];

    if (!peer_id || !group_id)
        return;

    snprintf (sql, 512, "DELETE FROM groups WHERE "
              "peer_id = '%s' AND group_id = '%s';",
              peer_id, group_id);
    ccnet_db_query (priv->relay_db, sql);
}

enum {
    RELAY_DB_COLUMN_PEER_ID = 0,
    RELAY_DB_COLUMN_GROUP_ID,
};

static gboolean get_group_cb (CcnetDBRow *row, void *data)
{
    GList **groups = (GList **)data;
    const char *group_id = (char *)ccnet_db_row_get_column_text(
        row, RELAY_DB_COLUMN_GROUP_ID);

    *groups = g_list_prepend (*groups, g_strdup(group_id));
    return TRUE;
}

GList*
ccnet_relay_manager_get_group_list (CcnetRelayManager *manager,
                                    const char* peer_id)
{
    RelayManagerPriv *priv = manager->priv;
    char sql[1024];
    int result;
    GList *groups = NULL;

    snprintf (sql, 1024, "SELECT * FROM groups WHERE peer_id = '%s';",
              peer_id);

    result = ccnet_db_foreach_selected_row (priv->relay_db, sql,
                                            get_group_cb, &groups);
    if (result < 0) {
        if (groups) {
            g_list_free (groups);
            groups = NULL;
        }
    }

    return groups;
}

static gboolean get_user_rolelist_cb (CcnetDBRow *row, void *data)
{
    GString *role_str = (GString *)data;
    char *user_id = (char *)ccnet_db_row_get_column_text (row, 0);
    char *roles = (char *)ccnet_db_row_get_column_text (row, 1);
    gint64 timestamp = (gint64)ccnet_db_row_get_column_int64 (row, 2);

    /* use "0" as a placeholder for no-role */
    if (!roles)
        roles = "0";
    g_string_append_printf (role_str,
                            "%s %s %"G_GUINT64_FORMAT"\n",
                            user_id, roles, timestamp);

    return TRUE;
}

GString *
ccnet_relay_manager_get_user_rolelist_str (CcnetRelayManager *manager,
                                           char *user_id)
{
    /* retrive the user's role info from relay-db, return it as a
     * fomatted string */

    CcnetDB *db = manager->priv->relay_db;
    GString *role_str = g_string_new (NULL);
    char sql[512];

    if (!user_id)
        return NULL;

    /* Can "ORDER BY user_id2" improve the whole sync-role-proc
     * process, since every time, in the client side, we will check
     * every user to see whether it is in relay-db? No, it's not
     * necessary since the users in client side are stored in a
     * hashtable.
     *
     */
    snprintf (sql, 512, "SELECT user_id2, roles, timestamp FROM "
              "RelayUserRole WHERE user_id1 = '%s';", user_id);
    ccnet_db_foreach_selected_row (db, sql, get_user_rolelist_cb, role_str);

    return role_str;
}


static gboolean get_timestamp_cb (CcnetDBRow *row, void *data)
{
    gint64 *timestamp = (gint64 *)data;
    *timestamp = (gint64)ccnet_db_row_get_column_int64 (row, 0);
    return FALSE;
}

static int
handle_user_role_entry (CcnetDB *db, UserRoleEntry *entry)
{
    /* process the UserRoleEntry :
     *
     * 1) update it if it has a newer timestamp
     * 2) add it to relay-db if not exists yet
     *
     */
    char sql[1024];
    gint64 timestamp;
    int result;

    if (!entry->user_id1 || !entry->user_id2)
        return 0;

    snprintf (sql, 1024, "SELECT timestamp FROM RelayUserRole WHERE "
              "user_id1 = '%s' and user_id2 = '%s';",
              entry->user_id1, entry->user_id2);

    result = ccnet_db_foreach_selected_row (db, sql,
                                            get_timestamp_cb,
                                            &timestamp);
    if (result == 0) {
        /* no info in the database yet */
        /* %Q will work when entry->roles is NULL */
        snprintf (sql, 1024, "INSERT INTO RelayUserRole VALUES ("
                  "'%s', '%s', '%s', %"G_GUINT64_FORMAT");",
                  entry->user_id1,
                  entry->user_id2,
                  entry->roles ? entry->roles : "",
                  entry->timestamp);
        ccnet_db_query (db, sql);
    } else {
        /* check again to ensure role info from client
           are newer than relay-db.

           This check should should be true 99.9% of all the time.
           If we surprisingly find that if failes, it means that
           anther peer of this user has just updated the role info
           when the relay is communicating with this peer.

           If so, we just let it go, because it will get in sync again
           in the next sync_pulse of this peer.

           To prevent this, we can consider to add a lock to lock this
           user's relay-db entries when a sync-role session begins.
        */
        if (timestamp < entry->timestamp) {
            snprintf (sql, 1024, "UPDATE RelayUserRole SET "
                      "roles = '%s', timestamp = %"G_GUINT64_FORMAT" "
                      "WHERE user_id1 = '%s' AND user_id2 = '%s';",
                      entry->roles ? entry->roles : "",
                      entry->timestamp,
                      entry->user_id1,
                      entry->user_id2);
            ccnet_db_query (db, sql);
        }
    }

    return 0;
}

int
ccnet_relay_manager_update_user_role_info (CcnetRelayManager *manager,
                                           GList *entry_list)
{
    /* update user role info in relay-db, @entry_list is parsed from
     * the content sent by client in sync-role processor */

    CcnetDB *db = manager->priv->relay_db;
    GList *ptr;
    UserRoleEntry *entry;
    
    g_assert (db);

    for (ptr = entry_list; ptr != NULL; ptr = ptr->next) {
        entry = (UserRoleEntry *) ptr->data;

        if (handle_user_role_entry (db, entry) != 0)
            break;
    }
    
    return 0;
}



static void
prune_role_info (CcnetRelayManager *manager)
{
    /* delete the entries in UseRole table of relay-db who satisfy :       
    ** 
    ** 1) "roles" column is NULL
    ** 2) "timestamp" column is more than one week old 
    ** 
    */

    CcnetDB *db = manager->priv->relay_db;
    char sql[1024];
    gint64 now = get_current_time();
    gint64 lower_boundary = now - USER_PRUNE_THRESHOD_SEC;

    ccnet_message ("[Relay Manager] begin to prune outdated "
                   "info in RelayUserRole table\n");

    /* you can't delete it with "roles = NULL" */
    snprintf (sql, 1024, "DELETE FROM RelayUserRole WHERE "
              "roles is NULL and timestamp < %"G_GUINT64_FORMAT";",
              lower_boundary);

    ccnet_db_query (db, sql);
    
    ccnet_message ("[Relay Manager] prune user info done.\n");
}

void
ccnet_relay_manager_prune_role_info (CcnetRelayManager *manager)
{
    /* run the prune_role_info routine every USER_PRUNE_PERIOD_SEC */

    CcnetDB *db = manager->priv->relay_db;
    gint64 next_time;
    gint64 now = get_current_time();
    int result;
    
    char sql[1024] = "SELECT next_time FROM RelayPruneTime;";
    result = ccnet_db_foreach_selected_row (db, sql, get_timestamp_cb,
                                            &next_time);
    memset(sql, 0, sizeof(sql));
    if (result == 0) {
        /* The table has just been created */
        next_time = now + (gint64)USER_PRUNE_PERIOD_SEC*(gint64)1000000;
        snprintf (sql, 1024, "INSERT INTO RelayPruneTime VALUES "
                  "(%"G_GUINT64_FORMAT");", next_time);

    } else if (now > next_time) {
        /* set the next time */
        next_time = now + (gint64)USER_PRUNE_PERIOD_SEC*(gint64)1000000;
        snprintf (sql, 1024, "UPDATE RelayPruneTime SET next_time "
                  "= %"G_GUINT64_FORMAT";", next_time);
    }

    if (sql[0]) {
        ccnet_db_query (db, sql);
        prune_role_info (manager);
    }
}
    
int 
ccnet_relay_manager_store_user_profile(CcnetRelayManager *mgr,
                                       CcnetUser *user,
                                       const char *data)
{
    CcnetDB *db = mgr->priv->relay_db;
    char sql[512];

    if (!user->id)
        return 0;

    snprintf (sql, 512, "SELECT user_id FROM UserProfile WHERE user_id = '%s'",
              user->id);
    gboolean exists = ccnet_db_check_for_existence (db, sql);
    gint64 now = get_current_time();

    if (exists) {
        snprintf (sql, 512, "UPDATE UserProfile SET profile = '%s', "
                  " timestamp = %"G_GINT64_FORMAT" "
                  " WHERE user_id = '%s'",
                  data ? data : "",
                  now, user->id);
    } else {
        snprintf (sql, 512, "INSERT INTO UserProfile VALUES ('%s', '%s', %"G_GINT64_FORMAT")",
                  user->id, data ? data : "", now);
    }

    int result = ccnet_db_query (db, sql);
    return result;
}

static gboolean get_user_profile_cb (CcnetDBRow *row, void *data)
{
    char **profile = (char **)data;
    *profile = g_strdup((char *)ccnet_db_row_get_column_text (row, 0));
    return FALSE;
}

char *
ccnet_relay_manager_get_user_profile(CcnetRelayManager *mgr,
                                     const char *user_id)
{
    char *profile = NULL;
    CcnetDB *db = mgr->priv->relay_db;
    char sql[512];
    if (!user_id)
        return NULL;
    snprintf (sql, 512, "SELECT profile FROM UserProfile WHERE user_id = '%s'",
              user_id);
    ccnet_db_foreach_selected_row (db, sql, get_user_profile_cb, &profile);
    return profile;
}
                                     
char *
ccnet_relay_manager_get_user_profile_timestamp(CcnetRelayManager *mgr,
                                               const char *user_id)
{
    gint64 timestamp = 0;
    char *timestamp_str = NULL;
    CcnetDB *db = mgr->priv->relay_db;
    char sql[512];

    if (!user_id)
        return NULL;

    snprintf (sql, 512, "SELECT timestamp FROM UserProfile WHERE user_id = '%s'",
              user_id);

    int result = ccnet_db_foreach_selected_row (db, sql, get_timestamp_cb,
                                                &timestamp);
    if (result > 0) {
        char buf[256];
        snprintf (buf, 256, "%"G_GINT64_FORMAT"", timestamp);
        timestamp_str = g_strdup(buf);
    }

    return timestamp_str;
}

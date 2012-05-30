/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "include.h"

#include <stdio.h>
#include "config-db.h"

static gboolean
get_value (sqlite3_stmt *stmt, void *data)
{
    char **p_value = data;

    *p_value = g_strdup((char *) sqlite3_column_text (stmt, 0));
    /* Only one result. */
    return FALSE;
}

char *
config_db_get_string (sqlite3 *config_db, const char *key)
{
    char sql[256];
    char *value = NULL;

    snprintf (sql, sizeof(sql), 
              "SELECT value FROM Config WHERE key='%s';",
              key);
    if (sqlite_foreach_selected_row (config_db, sql,
                                     get_value, &value) < 0)
        return NULL;

    return value;
}

int
config_db_get_int (sqlite3 *config_db,
                   const char *key,
                   gboolean *exists)
{
    char *value;
    int ret;

    value = config_db_get_string (config_db, key);
    if (!value) {
        *exists = FALSE;
        return -1;
    }

    ret = atoi (value);
    g_free (value);
    return ret;
}

int
config_db_set_string (sqlite3 *config_db,
                      const char *key,
                      const char *value)
{
    char sql[256];

    snprintf (sql, sizeof(sql),
              "REPLACE INTO Config VALUES ('%s', '%s');",
              key, value);
    if (sqlite_query_exec (config_db, sql) < 0)
        return -1;

    return 0;
}

int
config_db_set_int (sqlite3 *config_db,
                   const char *key,
                   int value)
{
    char sql[256];

    snprintf (sql, sizeof(sql),
              "REPLACE INTO Config VALUES ('%s', %d);",
              key, value);
    if (sqlite_query_exec (config_db, sql) < 0)
        return -1;

    return 0;
}

sqlite3 *
config_db_open_db (const char *db_path)
{
    sqlite3 *db;

    if (sqlite_open_db (db_path, &db) < 0)
        return NULL;

    /*
     * Values are stored in text. You should convert it
     * back to integer if needed when you read it from
     * db.
     */
    char *sql = "CREATE TABLE IF NOT EXISTS Config ("
        "key TEXT PRIMARY KEY, "
        "value TEXT);";
    sqlite_query_exec (db, sql);

    return db;
}

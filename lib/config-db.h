/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef CONFIG_DB_H
#define CONFIG_DB_H

#include "db.h"

/*
 * Returns: config value in string. The string should be freed by caller. 
 */
char *
config_db_get_string (sqlite3 *config_db,
                      const char *key);

/*
 * Returns:
 * If key exists, @exists will be set to TRUE and returns the value;
 * otherwise, @exists will be set to FALSE and returns -1.
 */
int
config_db_get_int (sqlite3 *config_db,
                   const char *key,
                   gboolean *exists);

int
config_db_set_string (sqlite3 *config_db,
                      const char *key,
                      const char *value);

int
config_db_set_int (sqlite3 *config_db,
                   const char *key, int value);

sqlite3 *
config_db_open_db (const char *db_path);

#endif

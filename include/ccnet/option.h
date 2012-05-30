/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * option.h: Including macros that used in both daemon and client library.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, 
 * Boston, MA 02111-1307, USA.
 */


#ifndef CCNET_OPTION_H
#define CCNET_OPTION_H

#include <stdio.h>
#include <glib.h>

#ifdef WIN32
static inline char *GetDeafaultDir()
{
    static char buf[128];
    static int inited = 0;

    if (!inited) {
        const char *home = g_get_home_dir();
        inited = 1;
        snprintf(buf, 128, "%s/ccnet", home);
    }
    return buf;
}

  #define DEFAULT_CONFIG_DIR GetDeafaultDir()
  #define CONFIG_FILE_NAME   "ccnet.conf"
  #define PREFS_FILE_NAME    "prefs.conf"
#else
  #define DEFAULT_CONFIG_DIR "~/.ccnet"
  #define CONFIG_FILE_NAME   "ccnet.conf"
  #define PREFS_FILE_NAME    "prefs.conf"
#endif

#define PEER_KEYFILE    "mykey.peer"

#define MAX_USERNAME_LEN 20
#define MIN_USERNAME_LEN 2

#define DEFAULT_PORT       10001

#define CHAT_APP      "Chat"
#define PEERMGR_APP   "PeerMgr"
#define GROUPMGR_APP  "GroupMgr"


enum {
    NET_STATUS_DOWN,
    NET_STATUS_INNAT,
    NET_STATUS_FULL
};

#endif

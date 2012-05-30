/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *
 * Copyright (C) 2008, 2009 Lingtao Pan
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


#ifndef CCNET_PEER_H
#define CCNET_PEER_H

#include <glib.h>
#include <glib-object.h>
#include <stdint.h>


#define CCNET_TYPE_PEER                  (ccnet_peer_get_type ())
#define CCNET_PEER(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CCNET_TYPE_PEER, CcnetPeer))
#define CCNET_IS_PEER(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CCNET_TYPE_PEER))
#define CCNET_PEER_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CCNET_TYPE_PEER, CcnetPeerClass))
#define CCNET_IS_PEER_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CCNET_TYPE_PEER))
#define CCNET_PEER_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CCNET_TYPE_PEER, CcnetPeerClass))


enum {
    PEER_DOWN,
    PEER_CONNECTED
};

enum {
    BIND_UNKNOWN,
    BIND_YES,
    BIND_NO
};

typedef struct _CcnetPeer CcnetPeer;
typedef struct _CcnetPeerClass CcnetPeerClass;

#define CCNET_PEERID_LEN  40


struct _CcnetPeer
{
    GObject       parent_instance;
    char          id[41];
    char          user_id[41];

    gint64        timestamp;

    char         *name;

    unsigned int  is_self : 1;
    unsigned int  can_connect : 1;
    unsigned int  in_local_network : 1;
    unsigned int  in_connection : 1;
    gboolean      login_started;
    char          *login_error;
    gboolean      logout_started;

    char         *public_addr;
    uint16_t      public_port;
    char         *service_url;        /* http server for relay in seaflie */

    char         *addr_str;
    uint16_t      port;

    int           net_state;

    GList        *role_list;

    GList        *myrole_list; /* my role on this user */

    gint8         bind_status;

};

struct _CcnetPeerClass
{
    GObjectClass    parent_class;
};


GType ccnet_peer_get_type (void);


CcnetPeer* ccnet_peer_new (const char *id);


#endif

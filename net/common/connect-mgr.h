/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
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

#ifndef CCNET_CONNECTION_MANAGER
#define CCNET_CONNECTION_MANAGER

#include <event.h>

#include "timer.h"

typedef struct CcnetConnManager CcnetConnManager;

struct CcnetConnManager
{
    CcnetSession    *session;

    CcnetTimer      *reconnect_timer;
    CcnetTimer      *listen_timer;
    CcnetTimer      *dnslookup_timer;

    evutil_socket_t  bind_socket;

#ifndef CCNET_SERVER
    /* multicast */
    evutil_socket_t  mcsnd_socket;
    evutil_socket_t  mcrcv_socket;
    struct sockaddr* mc_sasend;
    int              mc_salen;
    struct event     mc_event;

    int              multicast_error : 1;
#endif

    GList           *conn_list;
};

CcnetConnManager *ccnet_conn_manager_new (CcnetSession *session);
void ccnet_conn_manager_start (CcnetConnManager *manager);
void ccnet_conn_manager_stop (CcnetConnManager *manager);
gboolean ccnet_conn_manager_connect_peer (CcnetConnManager *manager,
                                          CcnetPeer *peer);
void ccnet_conn_manager_add_to_conn_list (CcnetConnManager *manager,
                                          CcnetPeer *peer);
void ccnet_conn_manager_remove_from_conn_list (CcnetConnManager *manager,
                                               CcnetPeer *peer);
void ccnet_conn_manager_cancel_conn (CcnetConnManager *manager,
                                     const char *addr, int port);

#endif

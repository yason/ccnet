/*
 * Copyright (C) 2010 GongGeng
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

#ifndef CCNETRPC_TRANPORT_H
#define CCNETRPC_TRANPORT_H

#include <ccnet.h>

typedef struct {
    CcnetClient *session;
    char  *peer_id;       /* NULL if local */
    char  *service;
} CcnetrpcTransportParam;        /* this structure will be parsed to
                                  * ccnet_transport_send ()
                                  */

typedef struct {
    CcnetClient *session;
    char  *peer_id;              /* NULL if local */
    char  *service;
} CcnetrpcAsyncTransportParam;   /* this structure will be parsed to
                                  * ccnet_async_transport_send ()
                                  */

char *ccnetrpc_transport_send (void *arg,
        const gchar *fcall_str, size_t fcall_len, size_t *ret_len);

int ccnetrpc_async_transport_send (void *arg, gchar *fcall_str,
                                 size_t fcall_len, void *rpc_priv);

#endif /* SEARPC_TRANPORT_H */

/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * Copyright (C) 2009 Lingtao Pan
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

#ifndef CCNET_HANDSHAKE_H
#define CCNET_HANDSHAKE_H

struct CcnetPacketIO;
typedef struct CcnetHandshake CcnetHandshake;

#include "peer.h"


typedef void (*handshakeDoneCB) (CcnetHandshake *handshake,
                                 CcnetPacketIO  *io,
                                 int             isConnected,
                                 const char     *peerId,
                                 void           *userData);

struct CcnetHandshake
{
    char   *id;                  /* the peer id */
    CcnetPeer             *peer;  /* only valid if it is outgoing */
    CcnetPacketIO         *io;
    struct CcnetSession *session;

    uint8_t state;

    handshakeDoneCB doneCB;
    void   *doneUserData;
};

CcnetHandshake* ccnet_handshake_new (CcnetSession *session,
                                     CcnetPeer *peer,
                                     CcnetPacketIO *io,
                                     handshakeDoneCB doneCB,
                                     void *doneUserData);


#endif

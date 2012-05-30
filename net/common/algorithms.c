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

#include "common.h"

#include "peer.h"
#include "session.h"
#include "peer-mgr.h"

#include "message.h"
#include "message-manager.h"

#include "proc-factory.h"
#include "processors/sendmsg-proc.h"

#include "algorithms.h"

#define DEBUG_FLAG CCNET_DEBUG_MESSAGE
#include "log.h"


#if 0
static void notify_sendmsg_success (CcnetSession *session, CcnetMessage *ack_msg, 
                                    char *destid)
{
    CcnetMessageManager *msg_mgr = session->msg_mgr;
    CcnetMessage *msg;
    char buf[128] = {0};

    g_assert (ack_msg != NULL);
    snprintf (buf, sizeof(buf), "%s %s", ack_msg->id, destid);
    msg = ccnet_message_new (session->base.id, ack_msg->from, ack_msg->app,
                             buf, FLAG_IS_ACK);
    if (memcmp(session->base.id, ack_msg->from, 40) == 0)
        ccnet_message_manager_add_msg (msg_mgr, msg, MSG_TYPE_SYS);
    else
        ccnet_send_message (session, msg);
}

static void msg_cb (CcnetProcessor *proc, gboolean success, void *data)
{
    CcnetMessage *msg = data;
    if (success) 
        notify_sendmsg_success (proc->session, msg, proc->peer->id);
}
#endif  /* 0 */

static void
ccnet_send_msg_to_peer (CcnetSession *session, CcnetPeer *peer,
                        CcnetMessage *msg)
{
    CcnetProcessor *processor;
    CcnetProcFactory *factory = session->proc_factory;

    g_assert (!peer->is_self);

    if (peer->net_state == PEER_CONNECTED) {
        processor = ccnet_proc_factory_create_master_processor 
            (factory, "send-msg", peer);
        g_assert (processor);

        ccnet_sendmsg_proc_set_msg (CCNET_SENDMSG_PROC(processor), msg);
        /* g_signal_connect (processor, "done", */
        /*                   G_CALLBACK(msg_cb), msg); */
        ccnet_processor_start (processor, 0, NULL);
    }
}

void
ccnet_send_message (CcnetSession *session, CcnetMessage *msg)
{
    CcnetPeer *peer;

    peer = ccnet_peer_manager_get_peer (session->peer_mgr, msg->to);
    if (peer) {
        if (peer->is_self)
            ccnet_message_manager_add_msg (session->msg_mgr,
                                           msg, MSG_TYPE_RECV);
        else
            ccnet_send_msg_to_peer (session, peer, msg);

        g_object_unref (peer);
    } else
        g_warning ("Invalid peer id %s\n", msg->to);
}


static gboolean
check_message_permission (CcnetSession *session, CcnetMessage *msg)
{
    CcnetPeerManager *peer_mgr = session->peer_mgr;
    CcnetPeer *from;

    if (strcmp(msg->app, IPEERMGR_APP) == 0)
        return TRUE;

    if (msg->flags & FLAG_IS_ACK)
        return TRUE;

    from = ccnet_peer_manager_get_peer (peer_mgr, msg->from);
    if (!from)
        return FALSE;
    /* peer to peer message */
    /* TODO: improve performance */
    if (ccnet_peer_has_role(from, "MyPeer") ||
        ccnet_peer_has_role(from, "MyRelay") ||
        ccnet_peer_has_role(from, "MyClient")) {
        g_object_unref (from);
        return TRUE;
    }
    g_object_unref (from);
        
    return FALSE;
}


int
ccnet_recv_message (CcnetSession *session, CcnetMessage *msg)
{
    CcnetMessageManager *msg_mgr = session->msg_mgr;

    if (check_message_permission(session, msg) == FALSE)
        return -1;

    ccnet_message_manager_add_msg (msg_mgr, msg, MSG_TYPE_RECV);

    return 0;
}

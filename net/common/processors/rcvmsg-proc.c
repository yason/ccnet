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

#include "common.h"

#include "peer.h"
#include "message.h"
#include "session.h"
#include "message-manager.h"
#include "peer-mgr.h"
#include "rcvmsg-proc.h"
#include "algorithms.h"

#define DEBUG_FLAG CCNET_DEBUG_MESSAGE
#include "log.h"

static int rcv_msg_start (CcnetProcessor *processor, int argc, char **argv);
static void handle_update (CcnetProcessor *processor, 
                           char *code, char *code_msg,
                           char *content, int clen);


G_DEFINE_TYPE (CcnetRcvmsgProc, ccnet_rcvmsg_proc, CCNET_TYPE_PROCESSOR)

static void
ccnet_rcvmsg_proc_class_init (CcnetRcvmsgProcClass *klass)
{
    CcnetProcessorClass *proc_class = CCNET_PROCESSOR_CLASS (klass);

    proc_class->name = "rcvmsg-proc";
    proc_class->start = rcv_msg_start;
    proc_class->handle_update = handle_update;
}

static void
ccnet_rcvmsg_proc_init (CcnetRcvmsgProc *processor)
{
}

static int rcv_msg_start (CcnetProcessor *processor, int argc, char **argv)
{
    ccnet_processor_send_response (processor, "200", "OK", NULL, 0);
    return 0;
}

static void handle_update (CcnetProcessor *processor,
                           char *code, char *code_msg,
                           char *content, int clen)
{
    CcnetMessage *msg;

    if (processor->peer->is_local) {
        msg = ccnet_message_from_string_local (content, clen);
        ccnet_send_message (processor->session, msg);
        ccnet_message_unref (msg);
    } else {
        msg = ccnet_message_from_string (content, clen);
        msg->rtime = time(NULL);
        ccnet_debug ("[msg] Received a message : %s - %.10s\n", 
                     msg->app, msg->body);

        int ret = ccnet_recv_message (processor->session, msg);
        if (ret == -1) {
            ccnet_message ("[msg] Message from %.8s permission error\n", 
                           msg->from);
            ccnet_processor_send_response (processor, SC_PERM_ERR,
                                           SS_PERM_ERR, NULL, 0);
            ccnet_processor_done (processor, TRUE);
            ccnet_message_unref (msg);
            return;
        }

        ccnet_message_unref (msg);
    }

    ccnet_processor_send_response (processor, "200", "OK", NULL, 0);
    ccnet_processor_done (processor, TRUE);
}

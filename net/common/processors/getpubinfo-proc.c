/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
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
#include "getpubinfo-proc.h"

#define DEBUG_FLAG CCNET_DEBUG_CONNECTION
#include "log.h"


/*
 * get peer's public info
 */

static int get_pubinfo_start (CcnetProcessor *processor, 
                                int argc, char **argv);
static void handle_response (CcnetProcessor *processor,
                             char *code, char *code_msg,
                             char *content, int clen);


G_DEFINE_TYPE (CcnetGetpubinfoProc, ccnet_getpubinfo_proc, CCNET_TYPE_PROCESSOR)

static void
ccnet_getpubinfo_proc_class_init (CcnetGetpubinfoProcClass *klass)
{
    CcnetProcessorClass *proc_class = CCNET_PROCESSOR_CLASS (klass);
    /* GObjectClass *gobject_class = G_OBJECT_CLASS (klass); */

    proc_class->name = "getpubinfo-proc";
    proc_class->start = get_pubinfo_start;
    proc_class->handle_response = handle_response;
}

static void
ccnet_getpubinfo_proc_init (CcnetGetpubinfoProc *processor)
{
}


static int get_pubinfo_start (CcnetProcessor *processor, 
                              int argc, char **argv)
{
    ccnet_processor_send_request (processor, "put-pubinfo");

    return 0;
}


static void parse_pubinfo (CcnetProcessor *processor,
                            char *content,
                            int clen)
{
    g_return_if_fail (content[clen-1] == '\0');
    ccnet_peer_update_from_string (processor->peer, content);
}

static void handle_response (CcnetProcessor *processor, 
                             char *code, char *code_msg,
                             char *content, int clen)
{
    if (code[0] != '2') {
        ccnet_warning ("[Getpubinfo] Receive bad response %s: %s\n",
                       code, code_msg);
        ccnet_processor_done (processor, FALSE);
        return;
    }

    parse_pubinfo (processor, content, clen);
    ccnet_processor_done (processor, TRUE);
}

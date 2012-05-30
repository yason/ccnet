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

#include <config.h>
#include <stdint.h>

#include <unistd.h>
#include <stdio.h>
/* #include <stdlib.h> */
#include <errno.h>
#include <string.h>

#include <dirent.h>
#include <sys/stat.h>
#include <glib.h>
#include <glib/gstdio.h>

#include "utils.h"
#include "marshal.h"

#include "ccnet-client.h"
#include "peer.h"
#include "group.h"
#include "group-mgr.h"


enum {
    ADDED_SIG,
    DELETING_SIG,
    UPDATED_SIG,
    OBJECT_ADDED_SIG,
    OBJECT_DELETING_SIG,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

#include "group-mgr-common.h"

static void
ccnet_group_manager_class_init (CcnetGroupManagerClass *klass)
{
    signals[ADDED_SIG] = 
        g_signal_new ("group-added", CCNET_TYPE_GROUP_MANAGER, 
                      G_SIGNAL_RUN_LAST,
                      0,        /* no class singal handler */
                      NULL, NULL, /* no accumulator */
                      g_cclosure_marshal_VOID__OBJECT,
                      G_TYPE_NONE, 1, G_TYPE_OBJECT);

    signals[DELETING_SIG] = 
        g_signal_new ("group-deleting", CCNET_TYPE_GROUP_MANAGER, 
                      G_SIGNAL_RUN_LAST,
                      0,        /* no class singal handler */
                      NULL, NULL, /* no accumulator */
                      g_cclosure_marshal_VOID__OBJECT,
                      G_TYPE_NONE, 1, G_TYPE_OBJECT);

    signals[UPDATED_SIG] = 
        g_signal_new ("group-updated", CCNET_TYPE_GROUP_MANAGER, 
                      G_SIGNAL_RUN_LAST,
                      0,        /* no class singal handler */
                      NULL, NULL, /* no accumulator */
                      g_cclosure_marshal_VOID__OBJECT,
                      G_TYPE_NONE, 1, G_TYPE_OBJECT);

    signals[OBJECT_ADDED_SIG] = 
        g_signal_new ("object-added", CCNET_TYPE_GROUP_MANAGER, 
                      G_SIGNAL_RUN_LAST,
                      0,        /* no class singal handler */
                      NULL, NULL, /* no accumulator */
                      g_cclosure_marshal_VOID__POINTER,
                      G_TYPE_NONE, 1, G_TYPE_POINTER);

    signals[OBJECT_DELETING_SIG] = 
        g_signal_new ("object-deleting", CCNET_TYPE_GROUP_MANAGER, 
                      G_SIGNAL_RUN_LAST,
                      0,        /* no class singal handler */
                      NULL, NULL, /* no accumulator */
                      g_cclosure_marshal_VOID__POINTER,
                      G_TYPE_NONE, 1, G_TYPE_POINTER);
}

static void
ccnet_group_manager_init (CcnetGroupManager *manager)
{
}

CcnetGroupManager*
ccnet_group_manager_new (CcnetClient *session)
{
    CcnetGroupManager* manager;

    manager = g_object_new (CCNET_TYPE_GROUP_MANAGER, NULL);

    manager->session = session;

    manager->group_id_hash = g_hash_table_new (g_str_hash, g_str_equal);
    manager->group_name_hash = g_hash_table_new (g_str_hash, g_str_equal);

    return manager;
}


gboolean
ccnet_group_manager_remove_group (CcnetGroupManager *manager, CcnetGroup *group)
{
    return remove_group_common (manager, group);
}




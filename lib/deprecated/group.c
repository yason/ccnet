/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "include.h"

#include <stdio.h>

#include <openssl/sha.h>

#include "ccnet-client.h"
#include "peer.h"
#include "user.h"
#include "group.h"
#include "group-mgr.h"
#include "utils.h"

#include "group-common.h"

CcnetGroup *
ccnet_group_new (const char *id, CcnetGroupManager *group_mgr)
{
    CcnetGroup *group;

    group = g_object_new (CCNET_TYPE_GROUP, NULL);

    if (id)
        group->id = g_strdup (id);
    else
        group->id = gen_uuid ();

    group->id_quark = g_quark_from_string (group->id);

    group->manager = group_mgr;

    return group;
}

gboolean
ccnet_group_has_member (CcnetGroup *group, const char *peer_id)
{
    return (g_hash_table_lookup (group->members, peer_id) != NULL);
}

gboolean
ccnet_group_has_follower (CcnetGroup *group, const char *peer_id)
{
    return (g_hash_table_lookup (group->followers, peer_id) != NULL);
}

gboolean
ccnet_group_is_a_maintainer (CcnetGroup *group, const char *peer_id)
{
    return (g_hash_table_lookup (group->maintainers, peer_id) != NULL);
}


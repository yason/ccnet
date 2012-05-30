/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "include.h"

#include <stdio.h>

#include "rsa.h"
#include "peer.h"
#include "utils.h"

#include "user.h"

#include "user-common.h"

static void
ccnet_user_class_init (CcnetUserClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;
    gobject_class->finalize = user_finalize;

    define_properties (gobject_class);
}

static void
ccnet_user_init (CcnetUser *factory)
{
}

CcnetUser*
ccnet_user_new (const char *id)
{
    CcnetUser *user;

    user = g_object_new (CCNET_TYPE_USER, NULL);
    memcpy (user->id, id, 40);
    user->id[40] = '\0';

    return user;
}


void
ccnet_user_set_roles (CcnetUser *user, const char *roles, gint64 timestamp)
{
    GList *role_list = string_list_parse_sorted (roles, ",");
    string_list_free (user->role_list);
    user->role_list = role_list;
    user->role_timestamp = timestamp;    
}

void
ccnet_user_set_myroles (CcnetUser *user, const char *roles, gint64 timestamp)
{
    GList *role_list = string_list_parse_sorted (roles, ",");
    string_list_free (user->myrole_list);
    user->myrole_list = role_list;
    user->myrole_timestamp = timestamp;
}

void
ccnet_user_get_roles_str (CcnetUser *user, GString* buf)
{
    string_list_join (user->role_list, buf, ",");
}

void
ccnet_user_get_myroles_str (CcnetUser *user, GString* buf)
{
    string_list_join (user->myrole_list, buf, ",");
}

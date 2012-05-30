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

#ifndef CCNET_GROUP_H
#define CCNET_GROUP_H


#include <glib-object.h>


#define CCNET_TYPE_GROUP                  (ccnet_group_get_type ())
#define CCNET_GROUP(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CCNET_TYPE_GROUP, CcnetGroup))
#define CCNET_IS_GROUP(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CCNET_TYPE_GROUP))
#define CCNET_GROUP_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CCNET_TYPE_GROUP, CcnetGroupClass))
#define CCNET_IS_GROUP_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CCNET_TYPE_GROUP))
#define CCNET_GROUP_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CCNET_TYPE_GROUP, CcnetGroupClass))

typedef struct _CcnetGroup CcnetGroup;
typedef struct _CcnetGroupClass CcnetGroupClass;

struct _CcnetGroupManager;

struct _CcnetGroup
{
    GObject       parent_instance;

    char         *id;                   /* uuid */
    GQuark        id_quark;
    char         *name;                 /* group name */
    char         *creator;
    guint64       timestamp;

    struct _CcnetGroupManager  *manager;

    char        *members_str;
    char        *followers_str;
    char        *maintainers_str;
    char        *rendezvous_str;

    GHashTable  *members;

    GHashTable  *followers;

    GHashTable  *maintainers;
    CcnetPeer   *rendezvous;
};

struct _CcnetGroupClass
{
    GObjectClass    parent_class;
};


GType ccnet_group_get_type (void);

CcnetGroup *ccnet_group_new (const char *id, struct _CcnetGroupManager *group_mgr);
void ccnet_group_free (CcnetGroup *group);

void ccnet_group_update_from_string (CcnetGroup *group, char *string);
CcnetGroup *ccnet_group_from_string (char *string,
                                     struct _CcnetGroupManager *manager);
GString *ccnet_group_to_string (CcnetGroup *group);
void ccnet_group_members_to_string_buf (CcnetGroup *group, GString *str);
void ccnet_group_maintainers_to_string_buf (CcnetGroup *group, GString *str);

gboolean ccnet_group_has_member (CcnetGroup *group, const char *peer_id);
gboolean ccnet_group_has_follower (CcnetGroup *group, const char *peer_id);
gboolean ccnet_group_is_a_maintainer (CcnetGroup *group, const char *peer_id);

#endif

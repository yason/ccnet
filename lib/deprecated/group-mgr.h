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

#ifndef CCNET_GROUP_MGR_H
#define CCNET_GROUP_MGR_H

#include <glib.h>
#include <glib-object.h>


#define CCNET_TYPE_GROUP_MANAGER                  (ccnet_group_manager_get_type ())
#define CCNET_GROUP_MANAGER(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CCNET_TYPE_GROUP_MANAGER, CcnetGroupManager))
#define CCNET_IS_GROUP_MANAGER(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CCNET_TYPE_GROUP_MANAGER))
#define CCNET_GROUP_MANAGER_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CCNET_TYPE_GROUP_MANAGER, CcnetGroupManagerClass))
#define CCNET_IS_GROUP_MANAGER_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CCNET_TYPE_GROUP_MANAGER))
#define CCNET_GROUP_MANAGER_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CCNET_TYPE_GROUP_MANAGER, CcnetGroupManagerClass))


typedef struct CcnetGroupManagerPriv CcnetGroupManagerPriv;

typedef struct _CcnetGroupManager CcnetGroupManager;
typedef struct _CcnetGroupManagerClass CcnetGroupManagerClass;

struct _CcnetClient;

struct _CcnetGroupManager
{
    GObject         parent_instance;

    struct _CcnetClient   *session;
    
    /* private */
    GHashTable            *group_id_hash;
    GHashTable            *group_name_hash;

    const char            *groupdb_path;

    CcnetGroupManagerPriv *priv;
};

struct _CcnetGroupManagerClass
{
    GObjectClass    parent_class;
};

GType ccnet_group_manager_get_type (void);

CcnetGroupManager*
    ccnet_group_manager_new (struct _CcnetClient *session);

void ccnet_group_manager_free (CcnetGroupManager *manager);
void ccnet_group_manager_clear (CcnetGroupManager *manager);

GList *ccnet_group_manager_get_groups (CcnetGroupManager *manager);

CcnetGroup *ccnet_group_manager_get_group (CcnetGroupManager *manager, 
                                           const char *group_id);

CcnetGroup *ccnet_group_manager_get_group_by_name (CcnetGroupManager *manager,
                                                   const char *group_name);

gboolean ccnet_group_manager_add_group (CcnetGroupManager *manager,
                                    CcnetGroup *group);

gboolean ccnet_group_manager_remove_group (CcnetGroupManager *manager,
                                       CcnetGroup *group);

#endif

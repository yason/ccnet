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

#ifndef CCNET_RELAY_MANAGER_H
#define CCNET_RELAY_MANAGER_H

#include <glib-object.h>

#define CCNET_TYPE_RELAY_MANAGER         (ccnet_relay_manager_get_type ())
#define CCNET_RELAY_MANAGER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CCNET_TYPE_RELAY_MANAGER, CcnetRelayManager))
#define CCNET_RELAY_MANAGER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), CCNET_TYPE_RELAY_MANAGER, CcnetRelayManagerClass))
#define CCNET_IS_RELAY_MANAGER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CCNET_TYPE_RELAY_MANAGER))
#define CCNET_IS_RELAY_MANAGER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), CCNET_TYPE_RELAY_MANAGER))
#define CCNET_RELAY_MANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), CCNET_TYPE_RELAY_MANAGER, CcnetRelayManagerClass))

typedef struct _CcnetRelayManager      CcnetRelayManager;
typedef struct _CcnetRelayManagerClass CcnetRelayManagerClass;

typedef struct RelayManagerPriv RelayManagerPriv;

struct _CcnetRelayManager {
    GObject  parent_instance;

    CcnetSession *session;

    RelayManagerPriv *priv;
};

struct _CcnetRelayManagerClass {
    GObjectClass parent_class;
};


GType ccnet_relay_manager_get_type (void);

CcnetRelayManager* ccnet_relay_manager_new (CcnetSession *session);
int ccnet_relay_manager_load_db (CcnetRelayManager *manager,
                                 const char *relay_db);

void ccnet_relay_manager_start (CcnetRelayManager *manager);

void ccnet_relay_manager_add_permission (CcnetRelayManager *manager,
                                         const char* relay_dst_id,
                                         const char* relay_src_id);

void ccnet_relay_manager_del_permission (CcnetRelayManager *manager,
                                         const char* relay_dst_id,
                                         const char* relay_src_id);

gboolean ccnet_relay_manager_check_permission (CcnetRelayManager *manager,
                                               const char* relay_dst_id,
                                               const char* relay_src_id);

GList* ccnet_relay_manager_get_peer_list (CcnetRelayManager *manager,
                                          const char* relay_dst_id);

void ccnet_relay_manager_add_group (CcnetRelayManager *manager,
                                    const char *peer_id,
                                    const char *group_id);

void ccnet_relay_manager_del_group (CcnetRelayManager *manager,
                                    const char* peer_id,
                                    const char* group_id);

GList* ccnet_relay_manager_get_group_list (CcnetRelayManager *manager,
                                           const char* peer_id);

GString *
ccnet_relay_manager_get_user_rolelist_str (CcnetRelayManager *relay_mgr,
                                            char *user_id);

int
ccnet_relay_manager_update_user_role_info (CcnetRelayManager *relay_mgr,
                                           GList *entry_list);

typedef struct _UserRoleEntry UserRoleEntry;

struct _UserRoleEntry {
    /*  user2 is "roles" to user1  */
    char *user_id1;
    char *user_id2;
    char *roles;
    gint64 timestamp;
};

   
void
ccnet_relay_manager_prune_role_info (CcnetRelayManager *manager);

int
ccnet_relay_manager_store_user_profile(CcnetRelayManager *mgr,
                                       struct _CcnetUser *user,
                                       const char *data);
char *
ccnet_relay_manager_get_user_profile(CcnetRelayManager *mgr,
                                     const char *user_id);
char *
ccnet_relay_manager_get_user_profile_timestamp(CcnetRelayManager *mgr,
                                               const char *user_id);
#endif

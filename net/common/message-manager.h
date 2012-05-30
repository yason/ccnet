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

#ifndef CCNET_MESSAGE_MANAGER_H
#define CCNET_MESSAGE_MANAGER_H

#include <glib-object.h>

#define CCNET_TYPE_MESSAGE_MANAGER         (ccnet_message_manager_get_type ())
#define CCNET_MESSAGE_MANAGER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CCNET_TYPE_MESSAGE_MANAGER, CcnetMessageManager))
#define CCNET_MESSAGE_MANAGER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), CCNET_TYPE_MESSAGE_MANAGER, CcnetMessageManagerClass))
#define CCNET_IS_MESSAGE_MANAGER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CCNET_TYPE_MESSAGE_MANAGER))
#define CCNET_IS_MESSAGE_MANAGER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), CCNET_TYPE_MESSAGE_MANAGER))
#define CCNET_MESSAGE_MANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), CCNET_TYPE_MESSAGE_MANAGER, CcnetMessageManagerClass))

typedef struct _CcnetMessageManager      CcnetMessageManager;
typedef struct _CcnetMessageManagerClass CcnetMessageManagerClass;

typedef struct MessageManagerPriv MessageManagerPriv;

struct _CcnetMessageManager {
	GObject  parent_instance;

    CcnetSession *session;

    MessageManagerPriv *priv;
};

struct _CcnetMessageManagerClass {
	GObjectClass parent_class;
};


GType ccnet_message_manager_get_type (void);

CcnetMessageManager* ccnet_message_manager_new (CcnetSession *session);

int ccnet_message_manager_start (CcnetMessageManager *manager);

int ccnet_message_manager_add_msg(CcnetMessageManager *manager,
                                  CcnetMessage *msg,
                                  int msg_type);

int ccnet_message_manager_subscribe_app (CcnetMessageManager *manager,
                                         CcnetProcessor *mq_proc,
                                         int n_app, char **apps);

int ccnet_message_manager_unsubscribe_app (CcnetMessageManager *manager,
                                           CcnetProcessor *mq_proc,
                                           int n_app, char **apps);


#endif

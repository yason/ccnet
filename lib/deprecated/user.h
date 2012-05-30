/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *
 * Copyright (C) 2011 GongGeng
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

#ifndef CCNET_USER_H
#define CCNET_USER_H

#include <glib-object.h>

#define CCNET_TYPE_USER                  (ccnet_user_get_type ())
#define CCNET_USER(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CCNET_TYPE_USER, CcnetUser))
#define CCNET_IS_USER(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CCNET_TYPE_USER))
#define CCNET_USER_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CCNET_TYPE_USER, CcnetUserClass))
#define CCNET_IS_USER_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CCNET_TYPE_USER))
#define CCNET_USER_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CCNET_TYPE_USER, CcnetUserClass))


typedef struct _CcnetUser CcnetUser;
typedef struct _CcnetUserClass CcnetUserClass;


struct _CcnetUser
{
    GObject       parent_instance;

    char          id[41];
    gint64        timestamp;

    char         *name;

    char         *relay_id;

    unsigned int  is_self : 1;

    GList        *role_list;
    gint64        role_timestamp;

    GList        *myrole_list; /* my role on this user */
    gint64        myrole_timestamp;
};

struct _CcnetUserClass
{
    GObjectClass    parent_class;
};

GType ccnet_user_get_type (void);

CcnetUser* ccnet_user_new (const char *id);

void
ccnet_user_get_roles_str (CcnetUser *user, GString *buf);

void
ccnet_user_get_myroles_str (CcnetUser *user, GString *buf);

#endif

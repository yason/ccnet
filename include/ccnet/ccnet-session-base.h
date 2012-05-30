/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * Copyright (C) 2010-2011 GongGeng
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

#ifndef CCNET_SESSION_BASE_H
#define CCNET_SESSION_BASE_H

#include <glib-object.h>

#define CCNET_TYPE_SESSION_BASE                  (ccnet_session_base_get_type ())
#define CCNET_SESSION_BASE(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CCNET_TYPE_SESSION_BASE, CcnetSessionBase))
#define CCNET_IS_SESSION_BASE(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CCNET_TYPE_SESSION_BASE))
#define CCNET_SESSION_BASE_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CCNET_TYPE_SESSION_BASE, CcnetSessionBaseClass))
#define CCNET_IS_SESSION_BASE_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CCNET_TYPE_SESSION_BASE))
#define CCNET_SESSION_BASE_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CCNET_TYPE_SESSION_BASE, CcnetSessionBaseClass))

typedef struct _CcnetSessionBase CcnetSessionBase;
typedef struct _CcnetSessionBaseClass CcnetSessionBaseClass;

struct _CcnetSessionBase {
    GObject         parent_instance;

    char            id[41];
    unsigned char   id_sha1[20];

    char           *user_name;

    char           *name;

    int             public_port;
    int             net_status;

    char           *service_url;
    char           *relay_id;
};


struct _CcnetSessionBaseClass {
    GObjectClass    parent_class;
};


GType ccnet_session_base_get_type (void);

CcnetSessionBase *ccnet_session_base_new (void);

#endif

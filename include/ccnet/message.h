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

#ifndef CCNET_MESSAGE_H
#define CCNET_MESSAGE_H


#include <glib.h>
#include <glib-object.h>

#define CCNET_TYPE_MESSAGE                  (ccnet_message_get_type ())
#define CCNET_MESSAGE(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CCNET_TYPE_MESSAGE, CcnetMessage))
#define CCNET_IS_MESSAGE(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CCNET_TYPE_MESSAGE))
#define CCNET_MESSAGE_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CCNET_TYPE_MESSAGE, CcnetMessageClass))
#define CCNET_IS_MESSAGE_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CCNET_TYPE_MESSAGE))
#define CCNET_MESSAGE_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CCNET_TYPE_MESSAGE, CcnetMessageClass))

typedef struct _CcnetMessage           CcnetMessage;
typedef struct _CcnetMessageClass      CcnetMessageClass;

#define FLAG_TO_GROUP 0x01
#define FLAG_IS_ERROR 0x02
#define FLAG_WITH_BLOOM 0x04
#define FLAG_IS_ACK 0x08
#define FLAG_IS_RENDEZVOUS 0x10
#define FLAG_TO_USER 0x20

struct _CcnetMessage
{
    GObject         parent_instance;

    char     flags;
    char    *id;        /* UUID */

    char     from[41];
    char     to[41];

    int      ctime;             /* creation time */
	int 	 rtime;             /* receive time */

    char    *app;               /* application */
    char    *body;
};

struct _CcnetMessageClass
{
    GObjectClass    parent_class;
};

GType ccnet_message_get_type (void);

CcnetMessage* ccnet_message_new (const char *from_id,
                                 const char *to_id,
                                 const char *app,
                                 const char *body,
                                 int flags);

CcnetMessage* ccnet_message_new_full (const char *from_id,
                                      const char *to_id,
                                      const char *app,
                                      const char *body,
                                      time_t ctime,
                                      time_t rcv_time,
                                      const char *msg_id,
                                      int flags);

void ccnet_message_free (CcnetMessage *msg);

void ccnet_message_to_string_buf (CcnetMessage *msg, GString *buf);
CcnetMessage *ccnet_message_from_string (char *buf, int len);

gboolean ccnet_message_is_to_group(CcnetMessage *msg);

/* to avoid string allocation */
inline void ccnet_message_body_take (CcnetMessage *msg, char *body);

inline void ccnet_message_body_dup (CcnetMessage *msg, char *body);

#endif

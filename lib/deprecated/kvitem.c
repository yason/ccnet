/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */


#include <config.h>

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#include <glib.h>

#include "utils.h"
#include "kvitem.h"
#include "string-util.h"


/* 
   kvitem format:

   v1 <timestamp> <category> <group-id> <id> <value>
   
 */


CcnetKVItem*
ccnet_kvitem_new (const char *category,
                  const char *group_id,
                  const char *id,
                  const char *value,
                  gint64 timestamp)
{
    CcnetKVItem *item = g_new0 (CcnetKVItem, 1);

    item->category = g_strdup (category);
    item->group_id = g_strdup (group_id);
    item->id = g_strdup (id);
    item->value = g_strdup (value);
    if (timestamp != 0)
        item->timestamp = timestamp;
    else
        item->timestamp = get_current_time();
    item->ref = 1;

    return item;
}

void
ccnet_kvitem_ref (CcnetKVItem *item)
{
    item->ref++;
}

void
ccnet_kvitem_unref (CcnetKVItem *item)
{
    if (--item->ref == 0)
        ccnet_kvitem_free (item);
}

void
ccnet_kvitem_free (CcnetKVItem *item)
{
    g_free (item->category);
    g_free (item->group_id);
    g_free (item->id);
    g_free (item->value);
    g_free (item);
}

void ccnet_kvitem_list_free (GList *list)
{
    GList *ptr;

    for (ptr = list; ptr; ptr = ptr->next) {
        ccnet_kvitem_unref (ptr->data);
    }
    g_list_free (list);
}

void
ccnet_kvitem_to_string_buf (CcnetKVItem *item, GString *buf)
{
    g_string_printf (buf, "v%d %"G_GINT64_FORMAT" %s %s %s %s",
                     KVITEM_VERSION,
                     item->timestamp,
                     item->category,
                     item->group_id,
                     item->id,
                     item->value);
}

CcnetKVItem *
ccnet_kvitem_from_string (char *buf)
{
    
    char *category, *group_id, *id, *value;
    char *p, *begin;
    gint64 timestamp;
    CcnetKVItem *item;
    int version;

    p = buf;
    sgoto_next(p);
    version = get_version(buf);
    if (version != KVITEM_VERSION)
        return NULL;

    begin = p;
    sgoto_next(p);
    timestamp = strtoll (begin, NULL, 10);
    
    category = p;
    sgoto_next(p);

    group_id = p;
    sgoto_next(p);

    id = p;
    sgoto_next(p);
    
    value = p;

    item = ccnet_kvitem_new (category, group_id, id, value, timestamp);
    return item;

error:
    return NULL;
}

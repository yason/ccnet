/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */


#ifndef CCNET_KVITEM_H
#define CCNET_KVITEM_H

#define KVITEM_VERSION 1

typedef struct _CcnetKVItem      CcnetKVItem;

struct _CcnetKVItem {
    int         ref;

    char       *category;
    char       *group_id;
    char       *id;
    char       *value;
    gint64      timestamp;
};


CcnetKVItem* ccnet_kvitem_new (const char *category,
                               const char *group_id,
                               const char *id,
                               const char *value,
                               gint64 timestamp);

void ccnet_kvitem_free (CcnetKVItem *item);

void ccnet_kvitem_ref (CcnetKVItem*);
void ccnet_kvitem_unref (CcnetKVItem*);

void ccnet_kvitem_list_free (GList *list);

void ccnet_kvitem_to_string_buf (CcnetKVItem *item, GString *buf);

CcnetKVItem *ccnet_kvitem_from_string (char *buf);



#endif

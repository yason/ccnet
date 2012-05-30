/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef CCNET_KVCLIENT_PROC_H
#define CCNET_KVCLIENT_PROC_H

#include <glib-object.h>

#include "processor.h"
#include "message.h"

#define CCNET_TYPE_KVCLIENT_PROC                  (ccnet_kvclient_proc_get_type ())
#define CCNET_KVCLIENT_PROC(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CCNET_TYPE_KVCLIENT_PROC, CcnetKvclientProc))
#define CCNET_IS_KVCLIENT_PROC(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CCNET_TYPE_KVCLIENT_PROC))
#define CCNET_KVCLIENT_PROC_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CCNET_TYPE_KVCLIENT_PROC, CcnetKvclientProcClass))
#define CCNET_IS_KVCLIENT_PROC_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CCNET_TYPE_KVCLIENT_PROC))
#define CCNET_KVCLIENT_PROC_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CCNET_TYPE_KVCLIENT_PROC, CcnetKvclientProcClass))

typedef struct _CcnetKvclientProc CcnetKvclientProc;
typedef struct _CcnetKvclientProcClass CcnetKvclientProcClass;

struct _CcnetKVItem;

typedef void (*KvitemGotCB) (struct _CcnetKVItem *item, void *data);

struct _CcnetKvclientProc {
    CcnetProcessor parent_instance;
    
    KvitemGotCB   kvitem_got_cb;
    void         *cb_data;
};

struct _CcnetKvclientProcClass {
    CcnetProcessorClass parent_class;
};

void ccnet_kvclient_proc_set_item_got_cb (CcnetKvclientProc *,
                                          KvitemGotCB, void *);

GType ccnet_kvclient_proc_get_type ();

void ccnet_kvclient_proc_put_item (CcnetKvclientProc *proc,
                                   struct _CcnetKVItem* item);


#endif

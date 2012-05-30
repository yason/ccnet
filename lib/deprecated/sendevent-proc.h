/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef CCNET_SENDEVENT_PROC_H
#define CCNET_SENDEVENT_PROC_H

#include <glib-object.h>

#include "processor.h"
#include "ccnet-object.h"

#define CCNET_TYPE_SENDEVENT_PROC                  (ccnet_sendevent_proc_get_type ())
#define CCNET_SENDEVENT_PROC(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CCNET_TYPE_SENDEVENT_PROC, CcnetSendeventProc))
#define CCNET_IS_SENDEVENT_PROC(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CCNET_TYPE_SENDEVENT_PROC))
#define CCNET_SENDEVENT_PROC_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CCNET_TYPE_SENDEVENT_PROC, CcnetSendeventProcClass))
#define IS_CCNET_SENDEVENT_PROC_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CCNET_TYPE_SENDEVENT_PROC))
#define CCNET_SENDEVENT_PROC_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CCNET_TYPE_SENDEVENT_PROC, CcnetSendeventProcClass))

typedef struct _CcnetSendeventProc CcnetSendeventProc;
typedef struct _CcnetSendeventProcClass CcnetSendeventProcClass;

struct _CcnetSendeventProc {
    CcnetProcessor parent_instance;
};

struct _CcnetSendeventProcClass {
    CcnetProcessorClass parent_class;
};

GType ccnet_sendevent_proc_get_type ();

struct _CcnetEvent;
int
ccnet_sendevent_proc_set_event (CcnetSendeventProc *sendevent_proc, 
                                struct _CcnetEvent *event);


#endif


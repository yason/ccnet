
#include <Python.h>
#include <sys/time.h>
#include <sys/types.h>

#include <glib-object.h>

#include <ccnet/cevent.h>
#include <structmember.h>


typedef struct CEventManagerObject { 
    PyObject_HEAD
    CEventManager *manager;
} CEventManagerObject;

static PyTypeObject CEventManager_Type;


static PyObject* CEventManager_New(PyTypeObject *type, PyObject *args,
    PyObject *kwargs)
{
    CEventManagerObject *self;
    assert (type != NULL && type->tp_alloc != NULL);
    self = (CEventManagerObject *)type->tp_alloc(type, 0);
    self->manager = cevent_manager_new();
    return (PyObject*)self;
}

/* EventBaseObject destructor */
static void CEventManager_Dealloc(CEventManagerObject *obj)
{
    obj->ob_type->tp_free((PyObject *)obj);
}


static PyObject* CEventManager_Start (CEventManagerObject *self,
                                      PyObject *args, 
                                      PyObject *kwargs)
{
    int result = cevent_manager_start(self->manager);
    return PyInt_FromLong(result);
}

static void __cevent_callback(CEvent *event, void *handler_data)
{
    PyObject *callback = handler_data;
    PyObject *event_data = event->data;
    PyObject *arglist;
    PyObject *result;

    arglist = Py_BuildValue("(iO)", event->id, event_data);
    result = PyObject_CallObject(callback, arglist);
    Py_DECREF(arglist);
    if (result)
        Py_DECREF(result);
    else {
        PyErr_Print();
    }
}

static PyObject* CEventManager_Register (CEventManagerObject *self,
                                         PyObject *args,
                                         PyObject *kwargs)
{
    uint32_t id;
    PyObject *callback;
    static char     *kwlist[] = {"callback", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O:CEventManager", kwlist,
                                     &callback))
        return NULL;

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "callback argument must be callable");
        return NULL;
    }
    
    id = cevent_manager_register (self->manager, __cevent_callback, callback);
    Py_INCREF(callback);
    
    return PyInt_FromLong(id);
}

static PyObject* CEventManager_AddEvent (CEventManagerObject *self,
                                         PyObject *args,
                                         PyObject *kwargs)
{
    int id;
    PyObject *event_data;
    static char     *kwlist[] = { "id", "data", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "iO:CEventManager", kwlist,
                                     &id, &event_data))
        return NULL;

    cevent_manager_add_event (self->manager, (uint32_t)id, event_data);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyGetSetDef CEventManager_Properties[] = {
    {NULL},
};

static PyMemberDef CEventManager_Members[] = {
    {NULL},
};

static PyMethodDef CEventManager_Methods[] = { 
    {"add_event",                (PyCFunction)CEventManager_AddEvent ,        
     METH_VARARGS|METH_KEYWORDS, "" }, 
    {"register",                 (PyCFunction)CEventManager_Register,    
     METH_VARARGS|METH_KEYWORDS, "" },
    {"start",              (PyCFunction)CEventManager_Start, 
     METH_VARARGS|METH_KEYWORDS, "" },
    {NULL, NULL, 0, NULL}
};

#ifdef WIN32
#define DEFERRED_ADDRESS(ADDR) 0
#else
#define DEFERRED_ADDRESS
#endif

static PyTypeObject CEventManager_Type = {
    PyObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type))
    0,                      
    "cevent.CEventManager",                    /*tp_name*/
    sizeof(CEventManagerObject),               /*tp_basicsize*/
    0,                                         /*tp_itemsize*/
    /* methods */
    (destructor)CEventManager_Dealloc,         /*tp_dealloc*/
    0,                                         /*tp_print*/
    0,                                         /*tp_getattr*/
    0,                                         /*tp_setattr*/
    0,                                         /*tp_compare*/
    0,                                         /*tp_repr*/
    0,                                         /*tp_as_number*/
    0,                                         /*tp_as_sequence*/
    0,                                         /*tp_as_mapping*/
    0,                                         /*tp_hash*/
    0,                                         /*tp_call*/
    0,                                         /*tp_str*/
    DEFERRED_ADDRESS(PyObject_GenericGetAttr), /*tp_getattro*/
    DEFERRED_ADDRESS(PyObject_GenericSetAttr), /*tp_setattro*/
    0,                                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  /*tp_flags*/
    0,                                         /*tp_doc*/
    0,                                         /*tp_traverse*/
    0,                                         /*tp_clear*/
    0,                                         /*tp_richcompare*/
    0,                                         /*tp_weaklistoffset*/
    0,                                         /*tp_iter*/
    0,                                         /*tp_iternext*/
    CEventManager_Methods,                     /*tp_methods*/
    CEventManager_Members,                     /*tp_members*/
    CEventManager_Properties,                  /*tp_getset*/
    0,                                         /*tp_base*/
    0,                                         /*tp_dict*/
    0,                                         /*tp_descr_get*/
    0,                                         /*tp_descr_set*/
    0,                                         /*tp_dictoffset*/
    0,                                         /*tp_init*/
    DEFERRED_ADDRESS(PyType_GenericAlloc),     /*tp_alloc*/
    CEventManager_New,                         /*tp_new*/
    DEFERRED_ADDRESS(PyObject_Del),            /*tp_free*/
    0,                                         /*tp_is_gc*/
};

static PyMethodDef CEventModule_Functions[] = { 
    {NULL},
};

DL_EXPORT(void) initcevent(void)
{
    PyObject       *m;
	
    m = Py_InitModule("cevent", CEventModule_Functions);

    if (PyType_Ready(&CEventManager_Type) < 0) { 
        return;
    }
    PyModule_AddObject(m, "CEventManager", (PyObject *)&CEventManager_Type);
}

#include <Python.h>
#include <structmember.h>
#include <glib-object.h>
#include <ccnet.h>


typedef struct {
    PyObject_HEAD
    CcnetClient *client;
    PyObject *response;
} PyCClientObject;

/* tp_init */
static int
PyCClient_init(PyCClientObject *self, PyObject *args, PyObject *kwargs)
{
    self->client = ccnet_client_new();
    return 0;
}

/* tp_dealloc */
static void
PyCClient_Dealloc(PyCClientObject *self)
{
    CcnetClient *client = self->client;
    
    if (client)
        g_object_unref (client);
    
    self->ob_type->tp_free((PyObject *)self);
    Py_XDECREF(self->response);
}

static PyObject *
PyCClient_load_confdir(PyCClientObject *self, PyObject *args)
{
    char *confdir = NULL;
    int ret;

    if (!PyArg_ParseTuple(args, "s:Ccnet.Client.load_confdir", &confdir))
        return NULL;

    ret = ccnet_client_load_confdir ((self->client), confdir);
    
    return PyInt_FromLong(ret);
}

static PyObject *
PyCClient_connect_daemon(PyCClientObject *self, PyObject *args, PyObject *kwargs)
{
    int ret;
    CcnetClientMode mode;
    static char *kwlist[] = {"mode", NULL}; 

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                                     "i:Ccnet.Client.connect_daemon",
                                     kwlist, &mode)) {
        return PyInt_FromLong(-1L);
    }

    ret = ccnet_client_connect_daemon (self->client, mode);
    return PyInt_FromLong(ret);
}

static PyObject *
PyCClient_get_request_id(PyCClientObject *self)
{
    int request_id;
    request_id = ccnet_client_get_request_id ((self->client));
    
    return PyInt_FromLong(request_id);
}

static PyObject *
PyCClient_send_request(PyCClientObject *self, PyObject *args)
{
    int request_id;
    const char *req = NULL;

    if (!PyArg_ParseTuple(args, "is:Ccnet.Client.send_request",
                          &request_id, &req))
        return NULL;

    if (!self->client->connected) {
        PyErr_SetString(PyExc_RuntimeError, "Not connected to daemon");
        return NULL;
    }

    ccnet_client_send_request(self->client, request_id, req);
    
    Py_RETURN_NONE;
}

static void
set_response_tuple (PyCClientObject *self, CcnetClient *client)
{
    Py_XDECREF(self->response);

    self->response = PyTuple_New(3);
    
    struct CcnetResponse *response = &client->response;
    char *code = response->code ? response->code : "";
    char *code_msg = response->code_msg ? response->code_msg : "";
    char *content;
    int clen = response->clen;
    
    PyTuple_SetItem(self->response, 0, PyString_FromString(code));
    PyTuple_SetItem(self->response, 1, PyString_FromString(code_msg));
    
    if (clen == 0) {
        PyTuple_SetItem(self->response, 2, PyString_FromString(""));
        
    } else {
        content = response->content ? response->content : "";
        if (content[clen-1] == '\0')
            PyTuple_SetItem(self->response, 2, PyString_FromString(content));
        else
            PyTuple_SetItem(self->response, 2,
                            PyString_FromStringAndSize(content, clen));
    }
}

static PyObject *
PyCClient_read_response (PyCClientObject *self)
{
    CcnetClient *client = self->client;
    int res;

    /* Release GIL in case it will be blocked for a long time */
    Py_BEGIN_ALLOW_THREADS
    res = ccnet_client_read_response(client);
    Py_END_ALLOW_THREADS

    if (res < 0)
        return PyInt_FromLong(res);

    set_response_tuple (self, client);
    
    return PyInt_FromLong(0L);
}

static PyObject *
PyCClient_send_update(PyCClientObject *self, PyObject *args)
{
    int request_id;
    const char *code;
    const char *reason;
    const char *content;
    int clen;

    if (!PyArg_ParseTuple(args, "isssi:Ccnet.Client.send_request",
                          &request_id, &code, &reason, &content, &clen))
        return NULL;

    if (!self->client->connected) {
        PyErr_SetString(PyExc_RuntimeError, "Not connected to daemon");
        return NULL;
    }

    ccnet_client_send_update (self->client, request_id,
                              code, reason, content, clen);
    
    Py_RETURN_NONE;
}

static PyObject *
PyCClient_send_cmd(PyCClientObject *self, PyObject *args)
{
    CcnetClient *client = self->client;
    const char *cmd;
    GError *error = NULL;

    if (!PyArg_ParseTuple(args, "s:Ccnet.Client.send_cmd", &cmd))
        return PyInt_FromLong(-1L);

    if (!self->client->connected) {
        PyErr_SetString(PyExc_RuntimeError, "Not connected to daemon");
        return NULL;
    }

    ccnet_client_send_cmd(client, cmd, &error);
    if (error) {
        PyErr_SetString(PyExc_RuntimeError, error->message);
        return NULL;
    }

    set_response_tuple (self, client);

    Py_RETURN_NONE;
}

/* Convert a ccnet message to a tuple. */
static PyObject *
msg_to_tuple(CcnetMessage *msg)
{
    g_assert(msg);
    /* type, body, timestamp*/
    PyObject *pymsg = PyTuple_New(3);

    PyTuple_SetItem(pymsg, 0, PyString_FromString(msg->app));
    PyTuple_SetItem(pymsg, 1, PyString_FromString(msg->body));
    PyTuple_SetItem(pymsg, 2, PyString_FromFormat("%d", msg->ctime));

    return pymsg;
}

static PyObject *
PyCClient_prepare_recv_message (PyCClientObject *self, PyObject *args)
{
    CcnetClient *client = self->client;
    const char *msg_type;
    int res = 0;

    if (!PyArg_ParseTuple(args, "s:Ccnet.Client.receive_message", &msg_type)) {
        return PyInt_FromLong(-1L);
    }

    Py_BEGIN_ALLOW_THREADS
    res = ccnet_client_prepare_recv_message(client, msg_type);
    Py_END_ALLOW_THREADS

    return PyInt_FromLong((long)res);
}

static PyObject *
PyCClient_receive_message (PyCClientObject *self)
{
    CcnetClient *client = self->client;
    CcnetMessage *msg = NULL;
    PyObject *pymsg = NULL;

    Py_BEGIN_ALLOW_THREADS
    msg = ccnet_client_receive_message(client);
    Py_END_ALLOW_THREADS

    if (!msg) {
        ccnet_client_disconnect_daemon (client);
        Py_RETURN_NONE;
    }

    pymsg = msg_to_tuple(msg);

    ccnet_message_free (msg);
    
    return pymsg;
}

static PyObject *
PyCClient_is_connected (PyCClientObject *self)
{
    long connected = 0;
    if (self->client && self->client->connected)
        connected = 1;
    
    return PyBool_FromLong(connected);
}


static PyMethodDef PyCClient_Methods[] = { 
    {"load_confdir",                (PyCFunction)PyCClient_load_confdir,
     METH_VARARGS,                  "" },
    {"connect_daemon",              (PyCFunction)PyCClient_connect_daemon,
     METH_VARARGS|METH_KEYWORDS,    "" },
    {"get_request_id",              (PyCFunction)PyCClient_get_request_id,
     METH_NOARGS,                   "" },
    {"is_connected",                (PyCFunction)PyCClient_is_connected,
     METH_NOARGS,                   "" },
    {"send_request",                (PyCFunction)PyCClient_send_request,
     METH_VARARGS,                  "" },
    {"read_response",               (PyCFunction)PyCClient_read_response,
     METH_NOARGS,                   "" },
    {"send_update",                 (PyCFunction)PyCClient_send_update,
     METH_VARARGS,                  "" },
    {"send_cmd",                    (PyCFunction)PyCClient_send_cmd,
     METH_VARARGS,                  "" },
    {"prepare_recv_message",        (PyCFunction)PyCClient_prepare_recv_message,
     METH_VARARGS,                  "" },
    {"receive_message",             (PyCFunction)PyCClient_receive_message,
     METH_NOARGS,                   "" },
    {NULL}
};

static PyMemberDef PyCClient_Members[] = {
    {"response", T_OBJECT_EX,
     offsetof(PyCClientObject, response), 0, "response from daemon" },
    {NULL}
};

static PyTypeObject PyCClient_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                      
    "cclient.Client",                          /*tp_name*/
    sizeof(PyCClientObject),                   /*tp_basicsize*/
    0,                                         /*tp_itemsize*/
    (destructor)PyCClient_Dealloc,             /*tp_dealloc*/
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
    0,                                         /*tp_getattro */
    0,                                         /*tp_setattro */
    0,                                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  /*tp_flags*/
    "python binding for ccnet client",         /*tp_doc*/
    0,                                         /*tp_traverse*/
    0,                                         /*tp_clear*/
    0,                                         /*tp_richcompare*/
    0,                                         /*tp_weaklistoffset*/
    0,                                         /*tp_iter*/
    0,                                         /*tp_iternext*/
    PyCClient_Methods,                         /*tp_methods*/
    PyCClient_Members,                         /*tp_members*/
    0,                                         /*tp_getset*/
    0,                                         /*tp_base*/
    0,                                         /*tp_dict*/
    0,                                         /*tp_descr_get*/
    0,                                         /*tp_descr_set*/
    0,                                         /*tp_dictoffset*/
    (initproc)PyCClient_init,                  /*tp_init*/
    0,                                         /*tp_alloc*/
    0,                                         /*tp_new*/
    0,                                         /*tp_free*/
    0,                                         /*tp_is_gc*/
};

static PyMethodDef PyCClientModule_Methods[] = { 
    {NULL}
};

static void
add_module_constant(PyObject *m)
{
    PyObject *sync_mode, *async_mode;
    
    sync_mode = PyInt_FromLong(CCNET_CLIENT_SYNC);
    async_mode = PyInt_FromLong(CCNET_CLIENT_ASYNC);
    
    PyModule_AddObject (m, "CLIENT_SYNC", sync_mode);
    PyModule_AddObject (m, "CLIENT_ASYNC", async_mode);
}

DL_EXPORT(void)
initcclient(void)
{
    g_type_init();

    PyObject *m;
    m = Py_InitModule("cclient", PyCClientModule_Methods);

    PyCClient_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&PyCClient_Type) < 0) { 
        return;
    }
    
    Py_INCREF (&PyCClient_Type);
    PyModule_AddObject(m, "Client", (PyObject *)&PyCClient_Type);

    add_module_constant(m);
}

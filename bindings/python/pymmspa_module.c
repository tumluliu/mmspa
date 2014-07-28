/*
 * =====================================================================================
 *
 *       Filename:  pymmspa.c
 *
 *    Description:  Python binding of multimodal shortest path algorithms 
 *
 *        Version:  0.1
 *        Created:  2014/07/25 16时21分41秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Lu LIU (lliu), nudtlliu@gmail.com
 *   Organization:  LfK@TUM
 *
 * =====================================================================================
 */
#include <Python.h>
#include "mmspa.h"

static PyObject *error;

static PyObject* py_echo_list(PyObject *self, PyObject *args)
{
    int i = 0;
    PyObject *tseq, *list;
    char* list_item;
    long t_seqlen=0;

    if (!PyArg_ParseTuple(args, "O", &list)) return NULL;

    tseq = PySequence_Fast(list, "argument must be iterable");
    if (!tseq) return NULL;

    t_seqlen = PySequence_Fast_GET_SIZE(tseq);

    for (i=0; i < t_seqlen; i++)
    {
        list_item = PyString_AsString(PySequence_Fast_GET_ITEM(tseq, i));
        if (!list_item) {
            PyErr_SetString(PyExc_TypeError, "only strings are supported");
            Py_DECREF(tseq);
            return NULL;
        }
        printf("%s \n", list_item);
    }

    Py_DECREF(tseq);
    Py_RETURN_NONE;
}

static PyObject* py_parse(PyObject* self, PyObject* args) {
    char* inputpath;
    int parse_result;
    if (!PyArg_ParseTuple(args, "s", &inputpath))
        return NULL;
    parse_result = Parse(inputpath);
    return Py_BuildValue("i", parse_result);
}

 
static PyObject* py_multimodal_twoq(PyObject* self, PyObject* args) {
    int i = 0;
    PyObject *tseq, *arg1;
    char *mode, *source;
    long t_seqlen=0;

    if (!PyArg_ParseTuple(args, "Os", &arg1, &source)) return NULL;

    tseq = PySequence_Fast(arg1, "argument must be iterable");
    if (!tseq) return NULL;

    t_seqlen = PySequence_Fast_GET_SIZE(tseq);

    const char* mode_list[t_seqlen];
    for (i=0; i < t_seqlen; i++)
    {
        mode = PyString_AsString(PySequence_Fast_GET_ITEM(tseq, i));
        if (!mode) {
            PyErr_SetString(PyExc_TypeError, "only strings are supported");
            Py_DECREF(tseq);
            return NULL;
        }
        mode_list[i] = mode;
    }

    /* call mmspa MultimodalTwoQ here */
    MultimodalTwoQ(mode_list, t_seqlen, source);

    Py_DECREF(tseq);
    Py_RETURN_NONE;
}

static PyObject* py_multimodal_bellmanford(PyObject* self, PyObject* args) {
    Py_RETURN_NONE;
}

static PyObject* py_dispose(PyObject* self, PyObject* args) {
    Dispose();
    Py_RETURN_NONE;
}

static PyObject* py_multimodal_dijkstra(PyObject* self, PyObject* args) {
    const char** mode_list;
    int mode_count;
    const char* source;
    Py_RETURN_NONE;
}

/*
 * Bind Python function names to our C functions
 */
static PyMethodDef py_methods[] = {
    {"echo_list", py_echo_list, METH_VARARGS},
    {"parse", py_parse, METH_VARARGS},
    {"mm_twoq", py_multimodal_twoq, METH_VARARGS},
    {"mm_dijkstra", py_multimodal_dijkstra, METH_VARARGS},
    {"mm_bellmanford", py_multimodal_bellmanford, METH_VARARGS},
    {"dispose", py_dispose, METH_VARARGS},
	{NULL, NULL}
};

/*
 * Python calls this to let us initialize our module
 */
PyMODINIT_FUNC initpymmspa(void) {
    PyObject *m;

    m = Py_InitModule("pymmspa", py_methods);
    if (m == NULL) return;

    error = PyErr_NewException("pymmspa.error", NULL, NULL);
    Py_INCREF(error);
    PyModule_AddObject(m, "error", error);
}

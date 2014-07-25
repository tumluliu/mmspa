/*
 * =====================================================================================
 *
 *       Filename:  pymmspa.c
 *
 *    Description:  Python binding of multimodal shortest path algorithms 
 *
 *        Version:  1.0
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

/* Except error handling variable */
static PyObject *error;

static PyObject* py_multimodal_dijkstra(PyObject* self, PyObject* args) {
    const char** mode_list;
    int mode_count;
    const char* source;
    Py_RETURN_NONE;
}

static PyObject* py_parse(PyObject* self, PyObject* args) {
    Py_RETURN_NONE;
}

static PyObject* py_multimodal_twoq(PyObject* self, PyObject* args) {
    Py_RETURN_NONE;
}

static PyObject* py_multimodal_bellmanford(PyObject* self, PyObject* args) {
    Py_RETURN_NONE;
}

static PyObject* sample_d(PyObject *self, PyObject *args)
{
    int i = 0;
    PyObject *tseq, *arg1;
    char* arg2;
    int t_seqlen=0;

    /* Check if a single argument was passed. We still don't know if it is
       a list/tuple/iterable. */

    if (!PyArg_ParseTuple(args, "O", &arg1)) return NULL;

    tseq = PySequence_Fast(arg1, "argument must be iterable");
    if (!tseq) return NULL;

    t_seqlen = PySequence_Fast_GET_SIZE(tseq);

    for (i=0; i < t_seqlen; i++)
    {
        arg2 = PyString_AsString(PySequence_Fast_GET_ITEM(tseq, i));
        if (!arg2) {
            PyErr_SetString(PyExc_TypeError, "only strings are supported");
            Py_DECREF(tseq);
            return NULL;
        }
        /* arg2 is a pointer to a null-terminated array of characters. It is
           not an array of pointers. So I don't think you should be trying to
           index it via "i". If you are trying to print just the first
           character of the string, then you should use:

               printf("%c \n", arg2[0])

           If you are trying to print the entire string, then you should use:

               printf("%s \n", arg2)
        */
        printf("%s \n", arg2);
    }

    Py_DECREF(tseq);

    Py_RETURN_NONE;
}

static PyMethodDef py_methods[] = {
    {"d", sample_d, METH_VARARGS, "sample function"},
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC initsample(void) {
    PyObject *m;

    m = Py_InitModule("sample", py_methods);
    if (m == NULL) return;

    error = PyErr_NewException("sample.error", NULL, NULL);
    Py_INCREF(error);
    PyModule_AddObject(m, "error", error);
}

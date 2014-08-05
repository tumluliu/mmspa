/*
 * =====================================================================================
 *
 *       Filename:  pymmspa4pg_module.c
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
#include "mmspa4pg/mmspa4pg.h"

static PyObject *error;

static PyObject* py_connect_db(PyObject* self, PyObject* args) {
    char* conn_info;
    int result;
    if (!PyArg_ParseTuple(args, "s", &conn_info)) return NULL;
    result = ConnectDB(conn_info);
    return Py_BuildValue("i", result);
}

static PyObject* py_disconnect_db(PyObject* self, PyObject* args) {
    DisconnectDB();
    Py_RETURN_NONE;
}

static PyObject* py_create_routing_plan(PyObject* self, PyObject* args) {
    int mode_count, public_mode_count;
    if (!PyArg_ParseTuple(args, "ii", &mode_count, &public_mode_count)) 
        return NULL;
    CreateRoutingPlan(mode_count, public_mode_count);
    Py_RETURN_NONE;
}

static PyObject* py_set_mode(PyObject* self, PyObject* args) {
    int index, mode_id;
    if (!PyArg_ParseTuple(args, "ii", &index, &mode_id)) return NULL;
    SetModeListItem(index, mode_id);
    Py_RETURN_NONE;
}

static PyObject* py_set_public_transit_mode(PyObject* self, PyObject* args) {
    int index, mode_id;
    if (!PyArg_ParseTuple(args, "ii", &index, &mode_id)) return NULL;
    SetPublicTransitModeSetItem(index, mode_id);
    Py_RETURN_NONE;
}

static PyObject* py_set_switch_condition(PyObject* self, PyObject* args) {
    int index;
    char* switch_condition;
    if (!PyArg_ParseTuple(args, "is", &index, &switch_condition)) return NULL;
    SetSwitchConditionListItem(index, switch_condition);
    Py_RETURN_NONE;
}

static PyObject* py_set_switching_constraint(PyObject* self, PyObject* args) {
    int index;
    VertexValidationChecker callback;
    /* I am so not sure if the parameter format string "O" is 
     * properly in this context 
     * by LIU Lu*/
    if (!PyArg_ParseTuple(args, "iO", &index, &callback)) return NULL;
    SetSwitchingConstraint(index, callback);
    Py_RETURN_NONE;
}

static PyObject* py_set_target_constraint(PyObject* self, PyObject* args) {
    VertexValidationChecker callback = NULL;
    /* I am so not sure if the parameter format string "O" is 
     * properly in this context 
     * by LIU Lu*/
    if (!PyArg_ParseTuple(args, "O", &callback)) return NULL;
    SetTargetConstraint(callback);
    Py_RETURN_NONE;
}

static PyObject* py_set_cost_factor(PyObject* self, PyObject* args) {
    char* cost_factor;
    if (!PyArg_ParseTuple(args, "s", &cost_factor)) return NULL;
    SetCostFactor(cost_factor);
    Py_RETURN_NONE;
}

static PyObject* py_get_final_path(PyObject* self, PyObject* args) {
    long long source, target;
    Path** path_result;
    if (!PyArg_ParseTuple(args, "LL", &source, &target)) return NULL;
    path_result = GetFinalPath(source, target);
    return Py_BuildValue("i", path_result);
}

static PyObject* py_get_final_cost(PyObject* self, PyObject* args) {
    long long target;
    const char* cost_field;
    double final_cost;
    if (!PyArg_ParseTuple(args, "Ls", &target, &cost_field)) return NULL;
    final_cost = GetFinalCost(target, cost_field);
    return Py_BuildValue("d", final_cost);
}

static PyObject* py_parse(PyObject* self, PyObject* args) {
    int parse_result;
    parse_result = Parse();
    return Py_BuildValue("i", parse_result);
}
 
static PyObject* py_multimodal_twoq(PyObject* self, PyObject* args) {
    long long source;
    if (!PyArg_ParseTuple(args, "L", &source)) return NULL;
    MultimodalTwoQ(source);
    Py_RETURN_NONE;
}

static PyObject* py_dispose_paths(PyObject* self, PyObject* args) {
    Path** paths;
    if (!PyArg_ParseTuple(args, "O", &paths)) return NULL;
    DisposePaths(paths);
    Py_RETURN_NONE;
}

static PyObject* py_dispose(PyObject* self, PyObject* args) {
    Dispose();
    Py_RETURN_NONE;
}

/*
 * Bind Python function names to our C functions
 */
static PyMethodDef py_methods[] = {
    {"connect_db", py_connect_db, METH_VARARGS},
    {"disconnect_db", py_disconnect_db, METH_VARARGS},
    {"create_routing_plan", py_create_routing_plan, METH_VARARGS},
    {"set_mode", py_set_mode, METH_VARARGS},
    {"set_public_transit_mode", py_set_public_transit_mode, METH_VARARGS},
    {"set_switch_condition", py_set_switch_condition, METH_VARARGS},
    {"set_switching_constraint", py_set_switching_constraint, METH_VARARGS},
    {"set_target_constraint", py_set_target_constraint, METH_VARARGS},
    {"set_cost_factor", py_set_cost_factor, METH_VARARGS},
    {"get_final_path", py_get_final_path, METH_VARARGS},
    {"get_final_cost", py_get_final_cost, METH_VARARGS},
    {"parse", py_parse, METH_VARARGS},
    {"multimodal_twoq", py_multimodal_twoq, METH_VARARGS},
    {"dispose_paths", py_dispose_paths, METH_VARARGS},
    {"dispose", py_dispose, METH_VARARGS},
	{NULL, NULL}
};

/*
 * Python calls this to let us initialize our module
 */
PyMODINIT_FUNC initpymmspa4pg(void) {
    PyObject *m;

    m = Py_InitModule("pymmspa4pg", py_methods);
    if (m == NULL) return;

    error = PyErr_NewException("pymmspa4pg.error", NULL, NULL);
    Py_INCREF(error);
    PyModule_AddObject(m, "error", error);
}

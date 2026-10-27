#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "checksum.h"

static PyObject *checksum(PyObject *self, PyObject *argument) {
    (void)self;
    char *data;
    Py_ssize_t size;
    if (PyBytes_AsStringAndSize(argument, &data, &size) < 0) {
        return NULL;
    }
    return PyLong_FromUnsignedLongLong(
        (unsigned long long)moving2arm_checksum((const unsigned char *)data, (size_t)size));
}

static PyMethodDef methods[] = {
    {"checksum", checksum, METH_O, "Compute an unsigned 64-bit FNV-1a checksum of bytes."},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef definition = {
    PyModuleDef_HEAD_INIT, "_moving2arm", "Native dependency porting exercise.",
    -1, methods, NULL, NULL, NULL, NULL
};

PyMODINIT_FUNC PyInit__moving2arm(void) {
    return PyModule_Create(&definition);
}

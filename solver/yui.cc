#include <Python.h>

#include <glog/logging.h>

static PyObject* hello(PyObject* self, PyObject* args) {
  LOG(INFO) << "Hello, World!";

  Py_INCREF(Py_None);
  return Py_None;
}

static PyMethodDef kExtMethods[] = {
  {"hello", (PyCFunction)hello, METH_VARARGS, NULL},
  {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC inityui() {
  google::InitGoogleLogging("yui");
  Py_InitModule("solver.yui", kExtMethods);
}

extern "C" {
#include <Python.h>
}

#include <string>

#include <music.hh>

#include "runtimetype.h"
#include "setuptype.h"

/* ----- Runtime data type ----- */

extern "C"
int
runtime_init(music_RuntimeObject *self, PyObject *args, PyObject *kwds)
{
  double timestep;
  PyObject *setupinstance;

  if (! PyArg_ParseTuple(args, "Od", &setupinstance, &timestep))
    return -1;

  music_SetupObject* setupinst = (music_SetupObject *)setupinstance;

  self->theRuntime = new MUSIC::runtime(setupinst->theSetup, timestep);

  return 0;
}


extern "C"
PyObject *
runtime_time(PyObject *self, PyObject *args)
{
  music_RuntimeObject* me = (music_RuntimeObject *) self;

  double time = me->theRuntime->time();

  return Py_BuildValue("d", time);
}


extern "C"
PyObject *
runtime_tick(PyObject *self, PyObject *args)
{
  music_RuntimeObject* me = (music_RuntimeObject *) self;

  me->theRuntime->tick();

  Py_INCREF(Py_None);
  return Py_None;
}


static PyMethodDef runtime_methods[] = {
  {"time", runtime_time, METH_NOARGS, "Simulation time"},
  {"tick", runtime_tick, METH_NOARGS, "Advance time"},
  {NULL, NULL, 0, NULL}        /* Sentinel */
};


extern "C"
void
Runtime_dealloc(music_RuntimeObject* self)
{
  /* Deallocate extra data structures here */
  delete self->theRuntime;

  self->ob_type->tp_free((PyObject*)self);

  fprintf(stderr, "Runtime object deallocated.\n");
}


static PyTypeObject music_RuntimeType = {
  PyObject_HEAD_INIT(NULL)
  0,                         /*ob_size*/
  "music.Runtime",             /*tp_name*/
  sizeof(music_RuntimeObject), /*tp_basicsize*/
  0,                         /*tp_itemsize*/
  (destructor)Runtime_dealloc, /*tp_dealloc*/
  0,                         /*tp_print*/
  0,                         /*tp_getattr*/
  0,                         /*tp_setattr*/
  0,                         /*tp_compare*/
  0,                         /*tp_repr*/
  0,                         /*tp_as_number*/
  0,                         /*tp_as_sequence*/
  0,                         /*tp_as_mapping*/
  0,                         /*tp_hash */
  0,                         /*tp_call*/
  0,                         /*tp_str*/
  0,                         /*tp_getattro*/
  0,                         /*tp_setattro*/
  0,                         /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT,        /*tp_flags*/
  "Runtime object",	     /* tp_doc */
  0,                         /* tp_traverse */
  0,                         /* tp_clear */
  0,                         /* tp_richcompare */
  0,                         /* tp_weaklistoffset */
  0,                         /* tp_iter */
  0,                         /* tp_iternext */
  runtime_methods,	     /* tp_methods */
  0,			     /* tp_members */
  0,                         /* tp_getset */
  0,                         /* tp_base */
  0,                         /* tp_dict */
  0,                         /* tp_descr_get */
  0,                         /* tp_descr_set */
  0,                         /* tp_dictoffset */
  (initproc)runtime_init,    /* tp_init */
  0,                         /* tp_alloc */
  0,			     /* tp_new */
};


/* ---------- Visible functions ---------- */

int init_runtime_type(void) {
  music_RuntimeType.tp_new = PyType_GenericNew;
  if (PyType_Ready(&music_RuntimeType) < 0)
    return 0;
  return 1;
}


void attach_runtime_type(PyObject* m) {
  Py_INCREF(&music_RuntimeType);
  PyModule_AddObject(m, "Runtime", (PyObject *)&music_RuntimeType);
}

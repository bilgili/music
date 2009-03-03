extern "C" {
#include <Python.h>
}

#include <string>

#include <music.hh>

#include "setuptype.h"

/* ----- Setup data type ----- */

extern "C"
int
setup_init(music_SetupObject *self, PyObject *args, PyObject *kwds)
{
  int argc;
  PyObject *py_argv;
  char **argv;

  if (! PyArg_ParseTuple(args, "iO", &argc, &py_argv))
    return -1;
  // FIXME: need to convert py_argv to argv
  self->theSetup = new MUSIC::setup(argc, argv);

  return 0;
}


extern "C"
PyObject *
setup_communicator(PyObject *self, PyObject *args)
{
  music_SetupObject* me = (music_SetupObject *) self;

  MPI::Intracomm comm = me->theSetup->communicator();

  // FIXME: should return wrapped version of comm

  Py_INCREF(Py_None);
  return Py_None;
}


extern "C"
PyObject *
setup_publish_cont_output(PyObject *self, PyObject *args)
{
  music_SetupObject* me = (music_SetupObject *) self;
  char* id;

  if (!PyArg_ParseTuple(args, "s", &id))
    return NULL;

  std::string c_id(id);

  me->theSetup->publish_cont_output(id);

  // FIXME: should return wrapped version of port

  Py_INCREF(Py_None);
  return Py_None;
}


static PyMethodDef setup_methods[] = {
  {"communicator", setup_communicator, METH_NOARGS, "Application-wide communicator"},
  {"publish_cont_output", setup_publish_cont_output,
   METH_VARARGS, "Create a new cont_putput_port"},
  {NULL, NULL, 0, NULL}        /* Sentinel */
};


extern "C"
void
Setup_dealloc(music_SetupObject* self)
{
  /* Deallocate extra data structures here */
  delete self->theSetup;

  self->ob_type->tp_free((PyObject*)self);

  fprintf(stderr, "Setup object deallocated.\n");
}


static PyTypeObject music_SetupType = {
  PyObject_HEAD_INIT(NULL)
  0,                         /*ob_size*/
  "music.Setup",             /*tp_name*/
  sizeof(music_SetupObject), /*tp_basicsize*/
  0,                         /*tp_itemsize*/
  (destructor)Setup_dealloc, /*tp_dealloc*/
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
  "Setup object",            /* tp_doc */
  0,                         /* tp_traverse */
  0,                         /* tp_clear */
  0,                         /* tp_richcompare */
  0,                         /* tp_weaklistoffset */
  0,                         /* tp_iter */
  0,                         /* tp_iternext */
  setup_methods,	     /* tp_methods */
  0,			     /* tp_members */
  0,                         /* tp_getset */
  0,                         /* tp_base */
  0,                         /* tp_dict */
  0,                         /* tp_descr_get */
  0,                         /* tp_descr_set */
  0,                         /* tp_dictoffset */
  (initproc)setup_init,	     /* tp_init */
  0,                         /* tp_alloc */
  0,			     /* tp_new */
};


/* ---------- Visible functions ---------- */

int init_setup_type(void) {
  music_SetupType.tp_new = PyType_GenericNew;
  if (PyType_Ready(&music_SetupType) < 0)
    return 0;
  return 1;
}


void attach_setup_type(PyObject* m) {
  Py_INCREF(&music_SetupType);
  PyModule_AddObject(m, "Setup", (PyObject *)&music_SetupType);
}

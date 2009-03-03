/* Glue to access Music from Python */

extern "C" {
#include <Python.h>
}

#include "mpi.h"
#include "music.hh"

#include "setuptype.h"
#include "runtimetype.h"

static PyMethodDef MusicMethods[] = {
  {NULL, NULL, 0, NULL}        /* Sentinel */
};


extern "C" PyMODINIT_FUNC
initmusic(void)
{
  PyObject* m;

  if (!init_setup_type())
    return;

  if (!init_runtime_type())
    return;

  m = Py_InitModule("music", MusicMethods);

  attach_setup_type(m);
  attach_runtime_type(m);
}

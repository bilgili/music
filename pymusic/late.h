#include <mpi.h>

#include "Python.h"

extern PyObject* (*wrapIntracomm_ptr) (MPI_Comm comm);

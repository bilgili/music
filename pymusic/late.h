#include <mpi.h>

#include "Python.h"

extern PyObject* (*make_Intracomm_ptr) (MPI_Comm comm);

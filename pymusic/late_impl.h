#include <mpi.h>

#include "Python.h"

PyObject* (*make_Intracomm_ptr) (MPI_Comm comm) = 0;

PyObject* make_Intracomm (MPI_Comm comm)
{
  return (*make_Intracomm_ptr) (comm);
}

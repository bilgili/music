from mpi4py.MPI cimport *
#from mpi4py.mpi_c cimport *

ctypedef object make_Intracomm_func (MPI_Comm comm)

cdef extern from "late.h":
    cdef make_Intracomm_func* make_Intracomm_ptr

# Local Variables:
# mode: python
# End:

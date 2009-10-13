from late cimport *

cdef make_Intracomm (MPI_Comm comm):
    cdef Intracomm icomm = Intracomm ()
    icomm.ob_mpi = comm
    return icomm

make_Intracomm_ptr = make_Intracomm

# Local Variables:
# mode: python
# End:

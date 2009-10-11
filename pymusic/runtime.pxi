from runtime cimport *

cdef class Runtime:
    cdef c_Runtime *thisptr      # hold a C++ instance which we're wrapping
    def __cinit__(self, Setup setup, h):
        self.thisptr = new_Runtime (setup.thisptr, h)
        setup.thisptr = NULL

    def __dealloc__(self):
        del_Runtime (self.thisptr)
        
    def tick (self):
        self.thisptr.tick ()

    def time (self):
        return self.thisptr.time ()

    def finalize (self):
        self.thisptr.finalize ()

# Local Variables:
# mode: python
# End:

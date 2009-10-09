cdef extern from "music/runtime.hh":
    ctypedef struct c_Runtime "MUSIC::Runtime":
        void tick ()
        double time ()
        void finalize ()
    c_Runtime *new_Runtime "new MUSIC::Runtime" (c_Setup* s, double h)
    void del_Runtime "delete" (c_Runtime *obj)

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

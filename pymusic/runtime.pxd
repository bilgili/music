from setup cimport *

cdef extern from "music/runtime.hh":
    ctypedef struct c_Runtime "MUSIC::Runtime":
        void tick ()
        double time ()
        void finalize ()
    c_Runtime *new_Runtime "new MUSIC::Runtime" (c_Setup* s, double h)
    void del_Runtime "delete" (c_Runtime *obj)

# Local Variables:
# mode: python
# End:

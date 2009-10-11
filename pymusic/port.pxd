import sys

cdef extern from "music/port.hh":
    ctypedef struct c_EventOutputPort "MUSIC::EventOutputPort":
        pass
    ctypedef struct c_EventInputPort "MUSIC::EventInputPort":
        pass

# Local Variables:
# mode: python
# End:

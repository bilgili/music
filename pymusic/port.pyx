import sys

#from port cimport *

cdef extern from "music/port.hh":
    ctypedef struct c_EventOutputPort "MUSIC::EventOutputPort":
        pass
    ctypedef struct c_EventInputPort "MUSIC::EventInputPort":
        pass

cdef wrapEventOutputPort (c_EventOutputPort* port):
    cdef EventOutputPort port_ = EventOutputPort ()
    port_.thisptr = port
    return port_

cdef class EventOutputPort:
    cdef c_EventOutputPort* thisptr
    def __cinit__(self):
        pass

cdef wrapEventInputPort (c_EventInputPort* port):
    cdef EventInputPort port_ = EventInputPort ()
    port_.thisptr = port
    return port_

cdef class EventInputPort:
    cdef c_EventInputPort* thisptr
    def __cinit__(self):
        pass

# Local Variables:
# mode: python
# End:

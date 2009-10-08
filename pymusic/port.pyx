import sys

from port cimport c_EventOutputPort, c_EventInputPort

cdef class EventOutputPort:
    cdef c_EventOutputPort* thisptr
    def __cinit__(self):
        pass

cdef class EventInputPort:
    cdef c_EventInputPort* thisptr
    def __cinit__(self):
        pass

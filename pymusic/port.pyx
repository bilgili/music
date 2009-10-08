import sys

from port cimport c_EventOutputPort

cdef class EventOutputPort:
    cdef c_EventOutputPort* thisptr
    def __cinit__(self):
        pass

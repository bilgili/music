import sys

from setup cimport *

cdef class Setup:
    cdef c_Setup* thisptr      # hold a C++ instance which we're wrapping
    def __cinit__(self, argv):
        # Convert argv into C array
        cdef int c_argc = 0
        cdef char* storage[100] #*fixme*
        cdef char** c_argv = storage
        for arg in argv:
            c_argv[c_argc] = arg
            c_argc += 1
            
        self.thisptr = new_Setup (c_argc, c_argv)

        # Fill argv with C array
        argv[:] = []
        for i in range (0, c_argc):
            argv.append (c_argv[i])
            
    def __dealloc__(self):
        del_Setup (self.thisptr)
        
    def publishEventOutput (self, identifier):
        return wrapEventOutputPort (self.thisptr.publishEventOutput (identifier))

    def publishEventInput (self, identifier):
        return wrapEventInputPort (self.thisptr.publishEventInput (identifier))

cdef c_Setup* unwrapSetup (Setup setup):
    return setup.thisptr

# Local Variables:
# mode: python
# End:

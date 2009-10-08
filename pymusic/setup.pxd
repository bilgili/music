cdef extern from "music/setup.hh":
    ctypedef struct c_Setup "MUSIC::Setup":
        c_EventOutputPort* publishEventOutput (char* identifier)
    c_Setup *new_Setup "new MUSIC::Setup" (int argc, char** argv)
    void del_Setup "delete" (c_Setup *obj)

cimport mpi4py.MPI as MPI
from mpi4py.mpi_c cimport *

from libcpp cimport bool as cbool
from libcpp.string cimport string

###########################################################

cdef extern from "music/data_map.hh" namespace "MUSIC":
    cdef cppclass CDataMap "MUSIC::DataMap":
        pass

cdef extern from "music/array_data.hh" namespace "MUSIC":
    cdef cppclass CArrayData "MUSIC::ArrayData"(CDataMap):
        CArrayData(void*, MPI_Datatype, int baseIndex, int size)

cdef extern from "music/port.hh" namespace "MUSIC":
    cdef cppclass CContInputPort "MUSIC::ContInputPort":
        void map(CDataMap*)

    cdef cppclass CContOutputPort "MUSIC::ContOutputPort":
        void map(CDataMap*)

cdef extern from "music/setup.hh" namespace "MUSIC":
    cdef cppclass CSetup "MUSIC::Setup":
        CSetup(int&, char**&) except +
        CSetup(int&, char**&, int, int*) except +

        cbool config(string, string*)
        cbool config(string, int*)
        cbool config(string, double*)

        CContInputPort*  publishContInput(string)
        CContOutputPort* publishContOutput(string)

cdef extern from "music/runtime.hh" namespace "MUSIC":
    cdef cppclass CRuntime "MUSIC::Runtime":
        CRuntime(CSetup*, double) except +
        void finalize()
        double time()
        void tick()

cdef extern from "music/music_c.h":
    cdef inline MPI_Comm communicator(CSetup*)
    cdef inline MPI_Comm communicator(CRuntime*)

###########################################################

cdef class Setup(object):
    cdef CSetup* ptr
    cdef int provided
    cdef MPI.Intracomm comm
    cdef set ports
    cdef void null(self)

###########################################################

cdef class Runtime(object):
    cdef CRuntime* ptr
    cdef MPI.Intracomm comm
    cdef set ports

###########################################################

cdef class ContInputPort(object):
    cdef CContInputPort* ptr
    cpdef object null(self)

cdef class ContOutputPort(object):
    cdef CContOutputPort* ptr
    cpdef object null(self)

###########################################################

from music.pybuffer cimport Buffer

cdef class DataMap(object):
    cdef CDataMap* ptr
    cdef Buffer buf

cdef class ArrayData(DataMap):
    pass

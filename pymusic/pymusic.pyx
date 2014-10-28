#
# distutils: language = c++
#

import cython
from libc.stdlib cimport malloc, free

import mpi4py.MPI as MPI

###########################################################

class MUSICError(Exception): pass

###########################################################
ctypedef struct Args:
    int argc
    char** argv

cdef Args argv_toc(list argv):
    cdef Args r
    r.argc = len(argv)
    if r.argc <= 0:
        raise MUSICError("argv can't be empty")

    r.argv = <char**> malloc((r.argc+1) * sizeof(char*))
    if r.argv is NULL:
        raise MUSICError("couldn't allocate argv")

    cdef string
    try:
        r.argv[r.argc] = NULL
        for i in range(0, r.argc):
            s = argv[i].encode()
            r.argv[i] = s
        return r
    except:
        free(r.argv)
        raise

###########################################################

cdef class ContInputPort(object):
    def __cinit__(self): self.ptr = NULL
    def __hash__(self): return <int> <size_t> self.ptr
    cpdef object null(self): self.ptr = NULL

    def map(self, DataMap d): self.ptr.map(d.ptr)

cdef class ContOutputPort(object):
    def __cinit__(self): self.ptr = NULL
    def __hash__(self): return <int> <size_t> self.ptr
    cpdef object null(self): self.ptr = NULL

    def map(self, DataMap d): self.ptr.map(d.ptr)

###########################################################

cdef class Setup(object):
    def __cinit__(self, argv, required=None):
        cdef int argc
        cdef char** argv_c
        cdef int provided
        cdef Args r = argv_toc(argv)
        try:
            if required is None:
                self.ptr = new CSetup(r.argc, r.argv)
                self.provided = MPI_THREAD_SINGLE
            else:
                self.ptr = new CSetup(r.argc, r.argv, required, &provided)
                self.provided = provided
        finally:
            free(r.argv)

        cdef MPI.Intracomm comm = MPI.Intracomm()
        comm.ob_mpi = communicator(self.ptr)
        self.comm = comm

        self.ports = set()

    def __dealloc__(self):
        if self.ptr is NULL:
            return

        del self.ptr
        for p in self.ports:
            del p.ptr
            p.null()
        self.null()

    cdef void null(self):
        self.ptr = NULL
        self.comm = <MPI.Intracomm> MPI.COMM_NULL
        self.ports = None

    def communicator(self): return self.comm

    def config(self, string var, type t):
        cdef:
            CSetup* s = self.ptr
            double vd
            int vi
            string vs

        if t is float:
            if not s.config(var, &vd):
                raise MUSICError("Config variable {} is not a double".format(var))
            return vd

        if t is int:
            if not s.config(var, &vi):
                raise MUSICError("Config variable {} is not an int".format(var))
            return vi

        if t is str:
            if not s.config(var, &vs):
                raise MUSICError("Config variable {} is not a string".format(var))
            return vs

        raise MUSICError("Config variable {} is requested as unknown type {}".format(var, t))

    def publishContInput(self, string s):
        cdef ContInputPort p = ContInputPort()
        p.ptr = self.ptr.publishContInput(s) # can't pass pointers directly to init
        self.ports.add(p)
        return p

    def publishContOutput(self, string s):
        cdef ContOutputPort p = ContOutputPort()
        p.ptr = self.ptr.publishContOutput(s)
        self.ports.add(p)
        return p

###########################################################

cdef class Runtime(object):
    def __cinit__(self, Setup setup, double h):
        self.ptr = new CRuntime(setup.ptr, h)
        self.ports = setup.ports
        setup.null()

        cdef MPI.Intracomm comm = MPI.Intracomm()
        comm.ob_mpi = communicator(self.ptr)
        self.comm = comm

    def __dealloc__(self):
        for p in self.ports:
            p.null()

        self.ptr.finalize()
        del self.ptr

    def communicator(self): return self.comm

    def time(self): return self.ptr.time()
    def tick(self): self.ptr.tick()

    def __iter__(self):
        cdef CRuntime* runtime = self.ptr
        while True:
            yield runtime.time()
            runtime.tick()

###########################################################

def _tslice_1(it, double stop):
    cdef double i
    for i in it:
        if i >= stop: return
        yield i

def _tslice_2(it, double start, double stop):
    _it = iter(it)
    cdef double i

    for i in _it:
        if i < start: continue
        if i >= stop: return
        break

    yield i
    for i in _it:
        if i >= stop: return
        yield i

def _tslice_3(it, double start, double stop, double step):
    _it = iter(it)
    cdef double i
    cdef double _i

    for i in _it:
        if i < start: continue
        if i >= stop: return
        break

    yield i; _i = i+step
    for i in _it:
        if i >= stop: return
        if i < _i: continue
        yield i; _i = i + step

def tslice(it, startstop, stop=None, step=None):
    if stop is None: return _tslice_1(it, <double>startstop)
    if step is None: return _tslice_2(it, <double>startstop, <double>stop)
    return _tslice_3(it, <double>startstop, <double>stop, <double>step)

###########################################################

from music.pybuffer import Buffer

cdef class DataMap(object):
    def __cinit__(self, object data, *args, **kwargs):
        self.buf = Buffer(data)
        self.ptr = NULL

    def __dealloc__(self):
        del self.ptr

cdef class ArrayData(DataMap):
    def __cinit__(self, object data, int index):
        cdef Buffer buf = self.buf
        self.ptr = new CArrayData(buf.pybuf.buf, 
                                  buf.dtype.ob_mpi, 
                                  index,
                                  buf.items)

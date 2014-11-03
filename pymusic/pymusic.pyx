#
# distutils: language = c++
#

from libc.stdlib cimport malloc, free

import mpi4py.MPI as MPI

###########################################################

class MUSICError(Exception): pass

###########################################################
ctypedef struct Args:
    #"""Just pass args as a named tuple"""
    int argc
    char** argv

cdef Args argv_toc(list argv):
    """Convert an argv python list to a correct C arg & argc"""
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

cdef class Port(object):
    """
    Base Port class.

    The underlying pointer is not deallocated here,
    since in MUSIC the port is owned by Setup/Runtime.
    This does add null when we believe the underlying object
    will be deallocated by the container.
    """
    def __cinit__(self): self.ptr = NULL
    def __hash__(self): return <int><size_t> self.ptr
    cpdef null(self): self.ptr = NULL

    def isConnected(self):
        return self.ptr.isConnected()

    def width(self):
        """
        Returns the width set in the MUSIC config file
        which defines the number of indices transferred
        along this port
        """
        if not self.ptr.hasWidth():
            raise MUSICError("No width defined")
        return self.ptr.width()

cdef class ContInputPort(Port):
    def map(self, object data, 
            int base=0, object perm=None, 
            double delay=0, cbool interpolate=True, int maxBuffered=-1):
        cdef Buffer buf = Buffer(data)
        cdef IndexMap imap = IndexMap(perm, base, buf.items)
        cdef DataMap d = DataMap(buf, imap)
        cdef CContInputPort* ptr = dc_CContInputPort(self.ptr)
        mapImpl(ptr, d.ptr, delay, maxBuffered, interpolate)

cdef class ContOutputPort(Port):
    def map(self, object data,
            int base=0, object perm=None,
            int maxBuffered=-1):
        cdef Buffer buf = Buffer(data)
        cdef IndexMap imap = IndexMap(perm, base, buf.items)
        cdef DataMap d = DataMap(buf, imap)
        cdef CContOutputPort* ptr = dc_CContOutputPort(self.ptr)
        mapImpl(ptr, d.ptr, maxBuffered)

cdef class EventInputPort(Port):
    """
    Maps 'events' to handlers.

    Since these 'events' must be protected from deallocation as long
    as the port exists (until finalization), we keep a set of them
    here and drop the reference when this object is null'd.
    """
    def __cinit__(self): 
        self.events = set()

    cpdef null(self):
        Port.null(self)
        self.events = None

    cdef _map(self, IndexType t,
              CEventInputPort* ptr, CIndexMap* imap, 
              CEventHandler* eh,
              double accLatency, int maxBuffered):        
        mapImpl(ptr, imap, t, getEventHandlerPtr(t, eh), 
                accLatency, maxBuffered)

    def map(self, func, IndexType t,
            double accLatency=0, int maxBuffered=-1,
            object perm=None, int base=-1, int size=-1):
        cdef IndexMap imap = IndexMap(perm, base, size)
        cdef EventHandler eh = EventHandler(func, t)
        cdef CEventInputPort* ptr = dc_CEventInputPort(self.ptr)
        self._map(t, ptr, imap.ptr, eh.ptr, accLatency, maxBuffered)
        self.events.add(eh)

cdef class EventOutputPort(Port):
    def map(self, IndexType t, int maxBuffered=-1,
            object perm=None, int base=-1, int size=-1):
        cdef IndexMap m = IndexMap(perm, base, size)
        cdef CEventOutputPort* ptr = dc_CEventOutputPort(self.ptr)
        mapImplH(ptr, m.ptr, t, maxBuffered)

    def insertEvent(self, double time, int index, IndexType t):
        cdef CEventOutputPort* ptr = dc_CEventOutputPort(self.ptr)
        insertEventImpl(ptr, time, index)

###########################################################

import sys
cdef class Setup(object):
    def __cinit__(self, argv=None, required=None):
        cdef int argc
        cdef char** argv_c
        cdef int provided

        cdef Args r = argv_toc(argv if argv is not None 
                               else sys.argv)
        try:
            if required is None:
                self.ptr = new CSetup(r.argc, r.argv)
                self.provided = MPI_THREAD_SINGLE
            else:
                self.ptr = new CSetup(r.argc, r.argv, required, &provided)
                self.provided = provided
            self.argv = [r.argv[i] for i in xrange(r.argc)]
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

    cdef null(self):
        self.ptr = NULL
        self.comm = <MPI.Intracomm> MPI.COMM_NULL
        self.ports = None

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

    def publishEventOutput(self, string s):
        cdef EventOutputPort p = EventOutputPort()
        p.ptr = self.ptr.publishEventOutput(s)
        self.ports.add(p)
        return p

    def publishEventInput(self, string s):
        cdef EventInputPort p = EventInputPort()
        p.ptr = self.ptr.publishEventInput(s)
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
        self.ptr.finalize()

        for p in self.ports:
            p.null()

        del self.ptr

    def time(self): return self.ptr.time()
    def tick(self): tick(self.ptr)        

    def __iter__(self):
        cdef CRuntime* ptr = self.ptr
        while True:
            yield ptr.time()
            tick(ptr)

##########################################################

from itertools import takewhile, dropwhile
def subrange(it, double start, double stop):
    return takewhile(lambda t: t < stop,
                     dropwhile(lambda t: t < start,
                               it))

###########################################################

from music.pybuffer import Buffer

from cpython cimport array
from array import array

cdef class IndexMap:
    def __cinit__(self, object perm=None, int base=0, int size=0):
        cdef array.array arr
        if perm is None:
            self.buf = None
            self.ptr = new LinearIndex(GlobalIndex(base), size)
        else:
            self.buf = Buffer(perm)
            if self.buf.dtype.size() != sizeof(int):
                arr = array('i', perm)
                self.buf = Buffer(arr)
            self.ptr = new PermutationIndex(<GlobalIndex*>
                                            self.buf.pybuf.buf,
                                            self.buf.items)

    def __dealloc__(self):
        del self.ptr

cdef class DataMap(object):
    def __cinit__(self, Buffer buf, IndexMap index_map=None, int index=0):
        self.buf = buf
        if index_map is None:
            self.ptr = new CArrayData(buf.pybuf.buf, 
                                      buf.dtype.ob_mpi, 
                                      index,
                                      buf.items)
        else:
            self.ptr = new CArrayData(buf.pybuf.buf,
                                      buf.dtype.ob_mpi,
                                      index_map.ptr)

    def __dealloc__(self):
        del self.ptr


cdef class EventHandler:
    def __cinit__(self, object func, IndexType t):
        if t == IndexGLOBAL:
            self.ptr = new CEHGlobal(<PyObject*>func)
        else:
            self.ptr = new CEHLocal(<PyObject*>func)
        self.func = func

    def __dealloc__(self): 
        del self.ptr


cdef class _Index:
    cdef readonly int GLOBAL
    cdef readonly int LOCAL

    def __cinit__(self):
        self.GLOBAL = IndexGLOBAL
        self.LOCAL = IndexLOCAL

Index = _Index()

cdef cbool EventCallback(PyObject* func, double d, IndexType t, int i) except False:
    (<object>func)(d, t, i)
    return True

pythonError = False
etype = NULL
evalue = NULL
etraceback = NULL

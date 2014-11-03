#ifndef MUSIC_C_H
#define MUSIC_C_H

#include <Python.h>

#include <mpi.h>
#include "music/setup.hh"
#include "music/runtime.hh"
#include "music/index_map.hh"
#include "music/event.hh"

#include <iostream>

namespace MUSIC {
  inline MPI_Comm communicator(MUSIC::Setup* s) {
    return (MPI_Comm) s->communicator();
  }

  inline MPI_Comm communicator(MUSIC::Runtime* r) {
    return (MPI_Comm) r->communicator();
  }

  inline int toint(MUSIC::GlobalIndex i) {
    return i;
  }

  inline int toint(MUSIC::LocalIndex i) {
    return i;
  }

  static bool pythonError;
  static PyObject* etype;
  static PyObject* evalue;
  static PyObject* etraceback;
  
  bool tick(Runtime* ptr) {
    ptr->tick();
    if (! pythonError) return true;

    pythonError = false;
    PyErr_Restore(etype, evalue, etraceback);
    return false;
  }

  bool EventCallback(PyObject* func, double d, Index::Type t, int index);

  class EventHandler { // All this just to insert a virtual d
  public:
    PyObject* const func;

    EventHandler(PyObject* func): func(func) {}
    virtual ~EventHandler() {}
    inline void callback(double d, Index::Type t, int i) {
      if (pythonError) return;
      if (EventCallback(func, d, t, i)) return;

      pythonError = true;
      PyErr_Fetch(&etype, &evalue, &etraceback);
    }
  };

  class EHLocal: public EventHandler, public EventHandlerLocalIndex {
  public:
    EHLocal(PyObject* func): EventHandler(func) {};
    void operator() (double d, LocalIndex i) {
      callback(d, Index::LOCAL, i);
    }
  };

  class EHGlobal: public EventHandler, public EventHandlerGlobalIndex {
  public:
    EHGlobal(PyObject* func): EventHandler(func) {};
    void operator() (double d, GlobalIndex i) {
      callback(d, Index::GLOBAL, i);
    }
  };

  inline EventHandlerPtr getEventHandlerPtr(Index::Type t, EventHandler* eh) {
    return (t == Index::GLOBAL)
      ? EventHandlerPtr((EHGlobal*) eh)
      : EventHandlerPtr((EHLocal*) eh);
  }

  class Implementer {
  public:
    static inline void mapImpl(ContInputPort* p, DataMap* d, 
			       double v, int i, bool b) {
      p->mapImpl(d, v, i, b);
    }
    static inline void mapImpl(ContOutputPort* p, DataMap* d, int i) {
      p->mapImpl(d, i);
    }
    static inline void mapImpl(EventInputPort* p, IndexMap* m, Index::Type t, 
			       EventHandlerPtr e, double d, int i) {
      p->mapImpl(m, t, e, d, i);
    }
    static inline void mapImplH(EventOutputPort* p, IndexMap* m, 
				Index::Type t, int i) {
      p->mapImplH(m, t, i);
    }
    static inline void insertEventImpl(EventOutputPort* p, double d, int i) {
      p->insertEventImpl(d, i);
    }
  };
}

#endif // MUSIC_C_H

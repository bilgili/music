/*
 *  This file is part of MUSIC.
 *  Copyright (C) 2008 INCF
 *
 *  MUSIC is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  MUSIC is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MUSIC_EVENT_HH

#include <music/index_map.hh>

namespace MUSIC {

  class Event {
  public:
    double t;
    int id;
    Event (double _t, int _id) : t (_t), id (_id) { }
    bool operator< (const Event& other) const { return t < other.t; }
  };

#if 0
  class EventFifo : public Fifo<Event> {
  public:
    void insert (int id, double t)
    {
      Event& s = Fifo<Event>::insert ();
      s.id = id;
      s.t = t;
    }
  };
#endif

  class EventHandlerGlobalIndex {
  public:
    virtual void operator () (double t, GlobalIndex id) = 0;
  };
  
  class EventHandlerGlobalIndexProxy
    : public EventHandlerGlobalIndex {
    void (*eventHandler) (double t, int id);
  public:
    EventHandlerGlobalIndexProxy () { }
    EventHandlerGlobalIndexProxy (void (*eh) (double t, int id))
      : eventHandler (eh) { }
    void operator () (double t, GlobalIndex id)
    {
      eventHandler (t, id);
    }
  };
  
  class EventHandlerLocalIndex {
  public:
    virtual void operator () (double t, LocalIndex id) = 0;
  };

  class EventHandlerLocalIndexProxy
    : public EventHandlerLocalIndex {
    void (*eventHandler) (double t, int id);
  public:
    EventHandlerLocalIndexProxy () { }
    EventHandlerLocalIndexProxy (void (*eh) (double t, int id))
      : eventHandler (eh) { }
    void operator () (double t, LocalIndex id)
    {
      eventHandler (t, id);
    }
  };

  class EventHandlerPtr {
    union {
      EventHandlerGlobalIndex* global;
      EventHandlerLocalIndex* local;
    } ptr;
  public:
    EventHandlerPtr (EventHandlerGlobalIndex* p) { ptr.global = p; }
    EventHandlerPtr (EventHandlerLocalIndex* p) { ptr.local = p; }
    EventHandlerGlobalIndex* global () { return ptr.global; }
    EventHandlerLocalIndex* local () { return ptr.local; }
  };
  
}

#define MUSIC_EVENT_HH
#endif

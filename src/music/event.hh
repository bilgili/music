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

  class event {
  public:
    double t;
    int id;
  };

#if 0
  class event_fifo : public fifo<event> {
  public:
    void insert (int id, double t)
    {
      event& s = fifo<event>::insert ();
      s.id = id;
      s.t = t;
    }
  };
#endif

  class event_handler_global_index {
  public:
    virtual void operator () (double t, global_index id) = 0;
  };
  
  class event_handler_global_index_proxy
    : public event_handler_global_index {
    void (*event_handler) (double t, int id);
  public:
    event_handler_global_index_proxy () { }
    event_handler_global_index_proxy (void (*eh) (double t, int id))
      : event_handler (eh) { }
    void operator () (double t, global_index id)
    {
      event_handler (t, id);
    }
  };
  
  class event_handler_local_index {
  public:
    virtual void operator () (double t, local_index id) = 0;
  };

  class event_handler_local_index_proxy
    : public event_handler_local_index {
    void (*event_handler) (double t, int id);
  public:
    event_handler_local_index_proxy () { }
    event_handler_local_index_proxy (void (*eh) (double t, int id))
      : event_handler (eh) { }
    void operator () (double t, local_index id)
    {
      event_handler (t, id);
    }
  };

  class event_handler_ptr {
    union {
      event_handler_global_index* global;
      event_handler_local_index* local;
    } ptr;
  public:
    event_handler_ptr (event_handler_global_index* p) { ptr.global = p; }
    event_handler_ptr (event_handler_local_index* p) { ptr.local = p; }
    event_handler_global_index* global () { return ptr.global; }
    event_handler_local_index* local () { return ptr.local; }
  };
  
}

#define MUSIC_EVENT_HH
#endif

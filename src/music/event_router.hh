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

#ifndef EVENT_ROUTER_HH

#include <vector>

#include <music/FIBO.hh>
#include <music/interval_tree.hh>
#include <music/index_map.hh>
#include <music/event.hh>

namespace MUSIC {

  class event_routing_data {
    index_interval _interval;
    FIBO* buffer;
  public:
    event_routing_data () { }
    event_routing_data (index_interval i, FIBO* b)
      : _interval (i), buffer (b) { }
    int begin () const { return _interval.begin (); }
    int end () const { return _interval.end (); }
    int offset () const { return _interval.local (); }
    void insert (double t, int id) {
      event* e = static_cast<event*> (buffer->insert ());
      e->t = t;
      e->id = id;
    }
  };


  class event_router {
    class inserter : public interval_tree<int, event_routing_data>::action {
    protected:
      double _t;
      int _id;
    public:
      inserter (double t, int id) : _t (t), _id (id) { };
      void operator() (event_routing_data& data)
      {
	data.insert (_t, _id - data.offset ());
      }
    };
    
    interval_tree<int, event_routing_data> routing_table;
  public:
    void insert_routing_interval (index_interval i, FIBO* b);
    void build_table ();
    void insert_event (double t, global_index id);
    void insert_event (double t, local_index id);
  };
    
}

#define EVENT_ROUTER_HH
#endif

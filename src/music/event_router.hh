/*
 *  This file is part of MUSIC.
 *  Copyright (C) 2008, 2009 INCF
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

#ifndef MUSIC_EVENT_ROUTER_HH

#include <vector>

#include <music/FIBO.hh>
#include <music/interval_tree.hh>
#include <music/index_map.hh>
#include <music/event.hh>

namespace MUSIC {

  class EventRoutingData {
    IndexInterval interval_;
    FIBO* buffer_;
  public:
    EventRoutingData () { }
    EventRoutingData (IndexInterval i, FIBO* b)
      : interval_ (i), buffer_ (b) { }
    int begin () const { return interval_.begin (); }
    int end () const { return interval_.end (); }
    void setEnd (int e) { interval_.setEnd (e); }
    int offset () const { return interval_.local (); }
    FIBO* buffer () const { return buffer_; }
    void insert (double t, int id) {
      Event* e = static_cast<Event*> (buffer_->insert ());
      e->t = t;
      e->id = id;
    }
    bool operator< (const EventRoutingData& data) const
    {
      return begin () < data.begin ();
    }
  };


  class EventRouter {
    class Inserter : public IntervalTree<int, EventRoutingData>::Action {
    protected:
      double t_;
      int id_;
    public:
      Inserter (double t, int id) : t_ (t), id_ (id) { };
      void operator() (EventRoutingData& data)
      {
	data.insert (t_, id_ - data.offset ());
      }
    };
    
    IntervalTree<int, EventRoutingData> routingTable;
  public:
    void insertRoutingInterval (EventRoutingData& data);
    void insertRoutingInterval (IndexInterval i, FIBO* b);
    void buildTable ();
    void insertEvent (double t, GlobalIndex id);
    void insertEvent (double t, LocalIndex id);
  };
    
}

#define MUSIC_EVENT_ROUTER_HH
#endif

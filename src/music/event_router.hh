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

#include <map>
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
    int offset () const { return interval_.local (); }
    void insert (double t, int id) {
      Event* e = static_cast<Event*> (buffer_->insert ());
      e->t = t;
      e->id = id;
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
    void insertRoutingInterval (IndexInterval i, FIBO* b);
    void buildTable ();
    void insertEvent (double t, GlobalIndex id);
    void insertEvent (double t, LocalIndex id);
  };
  /*
    * remedius
    */
  class CommonEventRoutingData {
     IndexInterval interval_;
     EventHandlerGlobalIndex *handleEvent_;
   public:
     CommonEventRoutingData () { }
     CommonEventRoutingData (IndexInterval i, EventHandlerGlobalIndex *handleEvent)
       : interval_ (i), handleEvent_ (handleEvent) { }
     int begin () const { return interval_.begin (); }
     int end () const { return interval_.end (); }
    // int offset () const { return interval_.local (); }
     void handle (double t, int id) {
    	 (*handleEvent_) (t, id);
     }
   };
  /*
   * remedius
   */
  class CommonEventRouter {
	  class EventHandler : public IntervalTree<int, CommonEventRoutingData>::Action {
		  double t_;
		  int id_;
	  public:
		  EventHandler (double t, int id) : t_ (t), id_ (id) { };
		  void operator() (CommonEventRoutingData& data)
		  {
			  data.handle(t_, id_);
		  }
	  };
	 int current;
     std::vector< IntervalTree<int, CommonEventRoutingData> > routingTables;
   public:
     CommonEventRouter():current(-1){};
     void newTable();
     void insertRoutingInterval (IndexInterval i, EventHandlerPtr *handleEvent);
     void buildTable ();
     void processEvent (double t, GlobalIndex id);
   };
    

  class EventRoutingMap {
    std::vector<Interval>* intervals;
    typedef std::map<FIBO*, std::vector<IndexInterval> > BufferMap;
    BufferMap bufferMap;
  public:
    EventRoutingMap () { intervals = new std::vector<Interval>; }
    ~EventRoutingMap () { delete intervals; }
    void insert (IndexInterval i, FIBO* buffer);
    void rebuildIntervals ();
    void fillRouter (EventRouter& router);
  };
}

#define MUSIC_EVENT_ROUTER_HH
#endif

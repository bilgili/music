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
  public:
    EventRoutingData () { }
    EventRoutingData (IndexInterval i): interval_ (i){ }
    virtual ~EventRoutingData(){};

    int begin () const { return interval_.begin (); }
    int end () const { return interval_.end (); }
    int offset () const { return interval_.local (); }
    virtual void *Data(){return NULL;}
    virtual void process (double t, int id){};
  };

  class InputRoutingData : public EventRoutingData{
	  EventHandlerGlobalIndex *handler;
  public:
	  InputRoutingData(IndexInterval i, EventHandlerGlobalIndex* h):EventRoutingData(i),handler (h){};
	  void *Data(){return handler;}
  	  void process (double t, int id) {
  		(*handler) (t, id);
  	  }
  };
  class OutputRoutingData: public EventRoutingData {
	  FIBO* buffer_;
  public:
	  OutputRoutingData(IndexInterval i, FIBO* b):EventRoutingData(i),buffer_ (b){};
	  void *Data(){return buffer_;}
	  void process (double t, int id) {
		  Event* e = static_cast<Event*> (buffer_->insert ());
		  e->t = t;
		  e->id = id;
	  }
  };

  class EventRouter {
  public:
      virtual ~EventRouter(){}
	  virtual void insertRoutingData (EventRoutingData& data){};
	  virtual void buildTable (){};
	  virtual void processEvent (double t, GlobalIndex id){};
	  virtual void processEvent (double t, LocalIndex id){};
  };
  class TableProcessingRouter: public EventRouter{
	  std::map<void*, int> indexer;
	  std::map<int, std::vector<EventRoutingData*> > routingTable;
  public:
	  TableProcessingRouter(){};
	  ~TableProcessingRouter();
	  void insertRoutingData (EventRoutingData& data);
	  void processEvent (double t, GlobalIndex id);
	  void processEvent (double t, LocalIndex id){};
  };
  class TreeProcessingRouter: public EventRouter{
	  class Processor : public IntervalTree<int, EventRoutingData>::Action {
	  protected:
		  double t_;
		  int id_;
	  public:
		  Processor (double t, int id) : t_ (t), id_ (id) { };
		  void operator() (EventRoutingData& data)
		  {
			  data.process (t_, id_ - data.offset ());
		  }
	  };

	  IntervalTree<int, EventRoutingData> routingTable;
	  public:
	  TreeProcessingRouter(){};
	  void insertRoutingData (EventRoutingData &data);
	  void buildTable ();
	  void processEvent (double t, GlobalIndex id);
	  void processEvent (double t, LocalIndex id);
  };

}
#define MUSIC_EVENT_ROUTER_HH
#endif

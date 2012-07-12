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
/* remedius
 * Since the router can act as on the output (point-to-point communication)
 * as well as on the input (collective communication) sides and
 * pre-/post-processing approaches are different
 * (in one case it's insertion to the buffer <FIBO>,
 * in another case it's calling appropriate handler <EventHandlerGlobalIndex>),
 * two more successors for EventRoutingData were created:
 * InputRoutingData and OutputRoutingData.
 * Also considering EventRoutingData partly realizes IndexInterval interface,
 * inheritance was introduced.
 */
  class EventRoutingData: public  IndexInterval{
  public:
    EventRoutingData () { }
    EventRoutingData (const IndexInterval &i):  IndexInterval (i.begin(), i.end(), i.local() ){ };
    virtual ~EventRoutingData(){};
    int offset () const { return this->local (); }
    virtual void *Data()=0;
    virtual void process (double t, int id)=0;
    virtual EventRoutingData *Clone() const =0;
  };

  class InputRoutingData : public EventRoutingData{
	  EventHandlerGlobalIndex *handler;
  public:
	  InputRoutingData(const IndexInterval &i, EventHandlerGlobalIndex* h):EventRoutingData(i),handler (h){};
	  void *Data(){return handler;}
  	  void process (double t, int id) {
  		(*handler) (t, id);
  	  }
  	 virtual EventRoutingData *Clone()const{return new InputRoutingData(*this, handler);}
  };
  class OutputRoutingData: public EventRoutingData {
	  FIBO* buffer_;
  public:
	  OutputRoutingData(const IndexInterval &i, FIBO* b):EventRoutingData(i),buffer_ (b){};
	  void *Data(){return buffer_;}
	  void process (double t, int id) {
		  Event* e = static_cast<Event*> (buffer_->insert ());
		  e->t = t;
		  e->id = id;
	  }
	  virtual EventRoutingData *Clone() const{return new OutputRoutingData(*this, buffer_);}
  };
/* remedius
 * We've decided to try different methods for pre-/post-processing the data:
 * using tree algorithm and using table.
 * Hence to this two EventRouter successors were created:
 * TableProcessingRouter and TreeProcessingRouter.
 * Currently TableProcessingRouter can handle only GlobalIndex processing.
 */
  class EventRouter {
  public:
      virtual ~EventRouter(){};
	  virtual void insertRoutingData (EventRoutingData& data){}
	  virtual void buildTable (){};
	  /* remedius
	   * insertEvent method was renamed to processEvent method,
	   * since we've introduced event processing on the input side as well.
	   */
	  virtual void processEvent (double t, GlobalIndex id){};
	  virtual void processEvent (double t, LocalIndex id){};
  };
  class TableProcessingRouter: public EventRouter{
	  //routingTable maps different buffers/even handlers of the current rank
	  //to the global index range, current rank is responsible for.
	  std::vector<EventRoutingData*> routingData; /*remove routingData*/
	  std::map<void*, std::vector<EventRoutingData*> > routingTable;
  public:
	  ~TableProcessingRouter();
	  void insertRoutingData (EventRoutingData& data);
	  void processEvent (double t, GlobalIndex id);
	  void processEvent (double t, LocalIndex id);
  };
  class TreeProcessingRouter: public EventRouter{
	  class Processor : public IntervalTree<int>::Action {
	  protected:
		  double t_;
		  int id_;
	  public:
		  Processor (double t, int id) : t_ (t), id_ (id) { };
		  void operator() (Interval &data)
		  {
			  ((EventRoutingData &)data).process (t_, id_ - ((EventRoutingData &)data).offset ());
		  }
	  };

	  IntervalTree<int> routingTable;
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

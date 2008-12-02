/*
 *  This file is part of MUSIC.
 *  Copyright (C) 2007, 2008 INCF
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

#ifndef MUSIC_PORT_HH

#include <string>

#include <music/data_map.hh>
#include <music/index_map.hh>
#include <music/event.hh>
#include <music/message.hh>
#include <music/event_router.hh>
#include <music/connectivity.hh>
#include <music/spatial.hh>

namespace MUSIC {

  class Setup;

  class Port {
  protected:
    Setup* _setup;
    ConnectivityInfo* _ConnectivityInfo;
    void assertOutput ();
    void assertInput ();
  public:
    Port () { }
    Port (Setup* s, std::string identifier);
    virtual void buildTable () { };
    virtual void setupCleanup () { };
    bool isConnected ();
    bool hasWidth ();
    int width ();
  };

  // A redistribution_port is a port with the ability to route
  // globally indexed data items from a sender MPI process to the
  // correct receiver MPI process.  Examples are cont_ports and
  // event_ports.
  
  class RedistributionPort : virtual public Port {
  protected:
  };

  class OutputPort : virtual public Port {
  };

  class InputPort : virtual public Port {
  };

  class OutputRedistributionPort : OutputPort {
  protected:
    SpatialOutputNegotiator* negotiator;
  public:
    void setupCleanup ();
  };

  class InputRedistributionPort : OutputPort {
  protected:
    SpatialInputNegotiator* negotiator;
  public:
    void setupCleanup ();
  };

  class ContPort : public RedistributionPort {
  };

  class ContOutputPort : public ContPort, public OutputRedistributionPort {
  public:
    ContOutputPort (Setup* s, std::string id)
      : Port (s, id) { }
    void map (DataMap* dmap);
    void map (DataMap* dmap, int maxBuffered);
  };
  
  class ContInputPort : public ContPort, public InputRedistributionPort {
  public:
    ContInputPort (Setup* s, std::string id)
      : Port (s, id) { }
    void map (DataMap* dmap, double delay = 0.0, bool interpolate = true);
    void map (DataMap* dmap, int maxBuffered, bool interpolate = true);
    void map (DataMap* dmap,
	      double delay,
	      int maxBuffered,
	      bool interpolate = true);
  };

  
  class EventPort : public RedistributionPort {
  };

  
  class EventOutputPort : public EventPort,
			    public OutputRedistributionPort {
    EventRouter router;
  public:
    EventOutputPort (Setup* s, std::string id)
      : Port (s, id) { }
    void map (IndexMap* indices, Index::Type type);
    void map (IndexMap* indices, Index::Type type, int maxBuffered);
    void mapImpl (IndexMap* indices, Index::Type type, int maxBuffered);
    void buildTable ();
    void insertEvent (double t, GlobalIndex id);
    void insertEvent (double t, LocalIndex id);
  };


  class EventInputPort : public EventPort,
			   public InputRedistributionPort {
  public:
    EventInputPort (Setup* s, std::string id)
      : Port (s, id) { }
    void map (IndexMap* indices,
	      EventHandlerGlobalIndex* handleEvent,
	      double accLatency = 0.0);
    void map (IndexMap* indices,
	      EventHandlerLocalIndex* handleEvent,
	      double accLatency = 0.0);
    void map (IndexMap* indices,
	      EventHandlerGlobalIndex* handleEvent,
	      double accLatency,
	      int maxBuffered);
    void map (IndexMap* indices,
	      EventHandlerLocalIndex* handleEvent,
	      double accLatency,
	      int maxBuffered);
    void mapImpl (IndexMap* indices,
		   Index::Type type,
		   EventHandlerPtr handleEvent,
		   double accLatency,
		   int maxBuffered);
    // Facilities to support the C interface
  public:
    EventHandlerGlobalIndexProxy*
    allocEventHandlerGlobalIndexProxy (void (*) (double, int));
    EventHandlerLocalIndexProxy*
    allocEventHandlerLocalIndexProxy (void (*) (double, int));
  private:
    EventHandlerGlobalIndexProxy cEventHandlerGlobalIndex;
    EventHandlerLocalIndexProxy cEventHandlerLocalIndex;
  };


  class MessagePort : virtual public Port {
  };

  class MessageOutputPort : public MessagePort, public OutputPort {
  public:
    void map ();
    void map (int maxBuffered);
  };

  class MessageInputPort : public MessagePort, public InputPort {
  public:
    void map (MessageHandler* handler, double accLatency = 0.0);
    void map (MessageHandler* handler, double accLatency, int maxBuffered);
  };

}

#define MUSIC_PORT_HH
#endif

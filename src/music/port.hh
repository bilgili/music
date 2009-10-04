/*
 *  This file is part of MUSIC.
 *  Copyright (C) 2007, 2008, 2009 INCF
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

#include <mpi.h>

#include <string>
#include <vector>

#include <music/data_map.hh>
#include <music/index_map.hh>
#include <music/event.hh>
#include <music/message.hh>
#include <music/connector.hh>
#include <music/sampler.hh>
#include <music/event_router.hh>
#include <music/connectivity.hh>
#include <music/spatial.hh>

namespace MUSIC {

  class Setup;

  class Port {
  public:
    Port () { }
    Port (Setup* s, std::string identifier);
    virtual void buildTable () { };
    virtual void setupCleanup () { };
    bool isConnected ();
    bool hasWidth ();
    int width ();

  protected:
    std::string portName_;
    Setup* setup_;
    ConnectivityInfo* ConnectivityInfo_;
    void assertOutput ();
    void assertInput ();

  private:
    void checkConnected (std::string action);
    bool isMapped_;
  };

  // A redistribution_port is a port with the ability to route
  // globally indexed data items from a sender MPI process to the
  // correct receiver MPI process.  Examples are cont_ports and
  // event_ports.
  
  class RedistributionPort : public virtual Port {
  };

  // A ticking port is a port which needs to be updated at every tick ()

  class TickingPort : public virtual Port {
  public:
    virtual void tick () = 0;
  };

  class OutputPort : public virtual Port {
  };

  class InputPort : public virtual Port {
  };

  class OutputRedistributionPort : public OutputPort,
				   public RedistributionPort {
  protected:
    SpatialOutputNegotiator* spatialNegotiator;
    virtual OutputConnector* makeOutputConnector (ConnectorInfo connInfo) = 0;
    virtual void mapImpl (IndexMap* indices,
			  Index::Type type,
			  int maxBuffered,
			  int dataSize);
  public:
    OutputRedistributionPort () : spatialNegotiator (0) { }
    void setupCleanup ();
  };

  class InputRedistributionPort : public OutputPort,
				  public RedistributionPort {
  protected:
    InputRedistributionPort () : spatialNegotiator (0) { }
    SpatialInputNegotiator* spatialNegotiator;
    virtual InputConnector* makeInputConnector (ConnectorInfo connInfo) = 0;
    void mapImpl (IndexMap* indices,
		  Index::Type type,
		  double accLatency,
		  int maxBuffered,
		  bool interpolate);
  public:
    void setupCleanup ();
  };

  class ContPort : virtual public Port {
  protected:
    Sampler sampler;
    MPI::Datatype type_;
  };

  class ContOutputPort : public ContPort,
			 public OutputRedistributionPort,
			 public TickingPort {
    void mapImpl (DataMap* indices, int maxBuffered);
    OutputConnector* makeOutputConnector (ConnectorInfo connInfo);
  public:
    ContOutputPort (Setup* s, std::string id)
      : Port (s, id) { }
    void map (DataMap* dmap);
    void map (DataMap* dmap, int maxBuffered);
    void tick ();
  };
  
  class ContInputPort : public ContPort, public InputRedistributionPort {
    double delay_;
    void mapImpl (DataMap* dmap,
		  double delay,
		  int maxBuffered,
		  bool interpolate);
    InputConnector* makeInputConnector (ConnectorInfo connInfo);
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

  
  class EventPort : public virtual Port {
  };

  
  class EventOutputPort : public EventPort,
			  public OutputRedistributionPort {
    std::vector<EventRoutingData>* routingData;
    EventRouter router;
  public:
    EventOutputPort (Setup* s, std::string id)
      : Port (s, id), routingData (new std::vector<EventRoutingData>) { }
    void map (IndexMap* indices, Index::Type type);
    void map (IndexMap* indices, Index::Type type, int maxBuffered);
    OutputConnector* makeOutputConnector (ConnectorInfo connInfo);
    void buildTable ();
    void insertEvent (double t, GlobalIndex id);
    void insertEvent (double t, LocalIndex id);
  };


  class EventInputPort : public EventPort,
			 public InputRedistributionPort {
  private:
    Index::Type type_;
    EventHandlerPtr handleEvent_;
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
  protected:
    void mapImpl (IndexMap* indices,
		  Index::Type type,
		  EventHandlerPtr handleEvent,
		  double accLatency,
		  int maxBuffered);
    InputConnector* makeInputConnector (ConnectorInfo connInfo);
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


  class MessagePort : public virtual Port {
  protected:
    int rank_;
  public:
    MessagePort (Setup* s);
  };

  class MessageOutputPort : public MessagePort,
			    public OutputRedistributionPort {
    std::vector<FIBO*> buffers;
  public:
    MessageOutputPort (Setup* s, std::string id);
    void map ();
    void map (int maxBuffered);
    void insertMessage (double t, void* msg, size_t size);
  protected:
    void mapImpl (int maxBuffered);
    OutputConnector* makeOutputConnector (ConnectorInfo connInfo);
  };

  class MessageInputPort : public MessagePort,
			   public InputRedistributionPort {
    MessageHandler* handleMessage_;
  public:
    MessageInputPort (Setup* s, std::string id);
    void map (MessageHandler* handler = 0, double accLatency = 0.0);
    void map (int maxBuffered);
    void map (double accLatency, int maxBuffered);
    void map (MessageHandler* handler, int maxBuffered);
    void map (MessageHandler* handler, double accLatency, int maxBuffered);
  protected:
    void mapImpl (MessageHandler* handleEvent,
		  double accLatency,
		  int maxBuffered);
    InputConnector* makeInputConnector (ConnectorInfo connInfo);
  public:
    MessageHandlerProxy*
    allocMessageHandlerProxy (void (*) (double, void*, size_t));
  private:
    MessageHandlerProxy cMessageHandler;
  };

}

#define MUSIC_PORT_HH
#endif

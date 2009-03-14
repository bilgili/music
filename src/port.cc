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

//#define MUSIC_DEBUG 1
#include "music/debug.hh"

#include "music/setup.hh" // Must be included first on BG/L
#include "music/port.hh"
#include "music/error.hh"

namespace MUSIC {

  Port::Port (Setup* s, std::string identifier)
    : portName_ (identifier), setup_ (s), isMapped_ (false)
  {
    ConnectivityInfo_ = s->portConnectivity (portName_);
    setup_->addPort (this);
  }


  bool
  Port::isConnected ()
  {
    return ConnectivityInfo_ != Connectivity::NO_CONNECTIVITY;
  }


  void
  Port::checkConnected (std::string action)
  {
    if (!isConnected ())
      {
	std::ostringstream msg;
	msg << "attempt to " << action << " port `" << portName_
	    << "' which is unconnected";
	error (msg);
      }
  }
  
  
  void
  Port::assertOutput ()
  {
    checkCalledOnce (isMapped_,
		     "OutputPort::map (...)",
		     " for port " + portName_);
    checkConnected ("map");
    if (ConnectivityInfo_->direction () != ConnectivityInfo::OUTPUT)
      {
	std::ostringstream msg;
	msg << "output port `" << ConnectivityInfo_->portName ()
	    << "' connected as input";
	error (msg);
      }
  }


  void
  Port::assertInput ()
  {
    checkCalledOnce (isMapped_,
		     "InputPort::map (...)",
		     " for port " + portName_);
    checkConnected ("map");
    if (ConnectivityInfo_->direction () != ConnectivityInfo::INPUT)
      {
	std::ostringstream msg;
	msg << "input port `" << ConnectivityInfo_->portName ()
	    << "' connected as output";
	error (msg);
      }
  }


  bool
  Port::hasWidth ()
  {
    checkConnected ("ask for width of");
    return ConnectivityInfo_->width () != ConnectivityInfo::NO_WIDTH;
  }


  int
  Port::width ()
  {
    checkConnected ("ask for width of");
    int w = ConnectivityInfo_->width ();
    if (w == ConnectivityInfo::NO_WIDTH)
      {
	std::ostringstream msg;
	msg << "width requested for port `" << ConnectivityInfo_->portName ()
	    << "' which has unspecified width";
	error (msg);
      }
    return w;
  }


  void
  OutputRedistributionPort::setupCleanup ()
  {
    // NOTE: Cleanup resources only used during setup phase,
    if (spatialNegotiator)
      delete spatialNegotiator;
  }
  

  void
  OutputRedistributionPort::mapImpl (IndexMap* indices,
				     Index::Type type,
				     int maxBuffered,
				     int dataSize)
  {
    // The timing algorithm in uses a different definition of
    // maxBuffered.  While the interface in port.hh counts the number
    // of "ticks of data" which must be stored, the timing algorithm
    // counts the offset in time between the processes in sender
    // ticks.
    if (maxBuffered != MAX_BUFFERED_NO_VALUE)
      maxBuffered -= 1;
	
    // Retrieve info about all remote connectors of this port
    PortConnectorInfo portConnections
      = ConnectivityInfo_->connections ();
    spatialNegotiator = new SpatialOutputNegotiator (indices, type);
    for (PortConnectorInfo::iterator info = portConnections.begin ();
	 info != portConnections.end ();
	 ++info)
      {
	// Create connector
	OutputConnector* connector = makeOutputConnector (*info);
	setup_->addConnection (new OutputConnection (connector,
						     maxBuffered,
						     dataSize));
      }
  }


  void
  InputRedistributionPort::setupCleanup ()
  {
    // NOTE: Cleanup resources only used during setup phase,
    if (spatialNegotiator)
      delete spatialNegotiator;
  }

  
  void
  InputRedistributionPort::mapImpl (IndexMap* indices,
				    Index::Type type,
				    double accLatency,
				    int maxBuffered,
				    bool interpolate)
  {
    // The timing algorithm in uses a different definition of
    // maxBuffered.  While the interface in port.hh counts the number
    // of "ticks of data" which must be stored, the timing algorithm
    // counts the offset in time between the processes in sender
    // ticks.
    if (maxBuffered != MAX_BUFFERED_NO_VALUE)
      maxBuffered -= 1;
	
    // Retrieve info about all remote connectors of this port
    PortConnectorInfo portConnections
      = ConnectivityInfo_->connections ();
    PortConnectorInfo::iterator info = portConnections.begin ();
    spatialNegotiator = new SpatialInputNegotiator (indices, type);
    InputConnector* connector = makeInputConnector (*info);
    ClockState integerLatency (accLatency, setup_->timebase ());
    setup_->addConnection (new InputConnection (connector,
						maxBuffered,
						integerLatency,
						interpolate));
  }

  
  /********************************************************************
   *
   * Cont Ports
   *
   ********************************************************************/
  
  void
  ContOutputPort::map (DataMap* dmap)
  {
    assertOutput ();
    int maxBuffered = MAX_BUFFERED_NO_VALUE;
    mapImpl (dmap, maxBuffered);
  }

  
  void
  ContOutputPort::map (DataMap* dmap, int maxBuffered)
  {
    assertOutput ();
    if (maxBuffered <= 0)
      {
	error ("ContOutputPort::map: maxBuffered should be a positive integer");
      }
    mapImpl (dmap, maxBuffered);
  }

  
  void
  ContOutputPort::mapImpl (DataMap* dmap,
			   int maxBuffered)
  {
    sampler.configure (dmap);
    type_ = dmap->type ();
    OutputRedistributionPort::mapImpl (dmap->indexMap (),
				       Index::GLOBAL,
				       maxBuffered,
				       0);
  }


  OutputConnector*
  ContOutputPort::makeOutputConnector (ConnectorInfo connInfo)
  {
    return new ContOutputConnector (connInfo,
				    spatialNegotiator,
				    setup_->communicator (),
				    sampler,
				    type_);
  }
  
  
  void
  ContOutputPort::tick ()
  {
    sampler.newSample ();
  }


  void
  ContInputPort::map (DataMap* dmap, double delay, bool interpolate)
  {
    assertInput ();
    int maxBuffered = MAX_BUFFERED_NO_VALUE;
    mapImpl (dmap,
	     delay,
	     maxBuffered,
	     interpolate);
  }

  
  void
  ContInputPort::map (DataMap* dmap,
		      int maxBuffered,
		      bool interpolate)
  {
    assertInput ();
    mapImpl (dmap,
	     0.0,
	     maxBuffered,
	     interpolate);
  }

  
  void
  ContInputPort::map (DataMap* dmap,
		      double delay,
		      int maxBuffered,
		      bool interpolate)
  {
    assertInput ();
    mapImpl (dmap,
	     delay,
	     maxBuffered,
	     interpolate);
  }

  
  void
  ContInputPort::mapImpl (DataMap* dmap,
			  double delay,
			  int maxBuffered,
			  bool interpolate)
  {
    sampler.configure (dmap);
    delay_ = delay;
    type_ = dmap->type ();
    InputRedistributionPort::mapImpl (dmap->indexMap (),
				      Index::GLOBAL,
				      delay,
				      maxBuffered,
				      interpolate);
  }

  
  InputConnector*
  ContInputPort::makeInputConnector (ConnectorInfo connInfo)
  {
    return new ContInputConnector (connInfo,
				   spatialNegotiator,
				   setup_->communicator (),
				   sampler,
				   type_,
				   delay_);
  }

  
  /********************************************************************
   *
   * Event Ports
   *
   ********************************************************************/
  
  void
  EventOutputPort::map (IndexMap* indices, Index::Type type)
  {
    assertOutput ();
    int maxBuffered = MAX_BUFFERED_NO_VALUE;
    mapImpl (indices, type, maxBuffered, sizeof (Event));
  }

  
  void
  EventOutputPort::map (IndexMap* indices,
			Index::Type type,
			int maxBuffered)
  {
    assertOutput ();
    if (maxBuffered <= 0)
      {
	error ("EventOutputPort::map: maxBuffered should be a positive integer");
      }
    mapImpl (indices, type, maxBuffered, sizeof (Event));
  }

  
  OutputConnector*
  EventOutputPort::makeOutputConnector (ConnectorInfo connInfo)
  {
    return new EventOutputConnector (connInfo,
				     spatialNegotiator,
				     setup_->communicator (),
				     router);
  }
  
  
  void
  EventOutputPort::buildTable ()
  {
    router.buildTable ();
  }

  
  void
  EventOutputPort::insertEvent (double t, GlobalIndex id)
  {
    router.insertEvent (t, id);
  }

  
  void
  EventOutputPort::insertEvent (double t, LocalIndex id)
  {
    router.insertEvent (t, id);
  }

  
  void
  EventInputPort::map (IndexMap* indices,
		       EventHandlerGlobalIndex* handleEvent,
		       double accLatency)
  {
    assertInput ();
    int maxBuffered = MAX_BUFFERED_NO_VALUE;
    mapImpl (indices,
	     Index::GLOBAL,
	     EventHandlerPtr (handleEvent),
	     accLatency,
	     maxBuffered);
  }

  
  void
  EventInputPort::map (IndexMap* indices,
		       EventHandlerLocalIndex* handleEvent,
		       double accLatency)
  {
    assertInput ();
    int maxBuffered = MAX_BUFFERED_NO_VALUE;
    mapImpl (indices,
	     Index::LOCAL,
	     EventHandlerPtr (handleEvent),
	     accLatency,
	     maxBuffered);
  }

  
  void
  EventInputPort::map (IndexMap* indices,
		       EventHandlerGlobalIndex* handleEvent,
		       double accLatency,
		       int maxBuffered)
  {
    assertInput ();
    if (maxBuffered <= 0)
      {
	error ("EventInputPort::map: maxBuffered should be a positive integer");
      }
    mapImpl (indices,
	     Index::GLOBAL,
	     EventHandlerPtr (handleEvent),
	     accLatency,
	     maxBuffered);
  }

  
  void
  EventInputPort::map (IndexMap* indices,
		       EventHandlerLocalIndex* handleEvent,
		       double accLatency,
		       int maxBuffered)
  {
    assertInput ();
    if (maxBuffered <= 0)
      {
	error ("EventInputPort::map: maxBuffered should be a positive integer");
      }
    mapImpl (indices,
	     Index::LOCAL,
	     EventHandlerPtr (handleEvent),
	     accLatency,
	     maxBuffered);
  }

  
  void
  EventInputPort::mapImpl (IndexMap* indices,
			   Index::Type type,
			   EventHandlerPtr handleEvent,
			   double accLatency,
			   int maxBuffered)
  {
    type_ = type;
    handleEvent_ = handleEvent;
    InputRedistributionPort::mapImpl (indices,
				      type,
				      accLatency,
				      maxBuffered,
				      false);
  }

  
  InputConnector*
  EventInputPort::makeInputConnector (ConnectorInfo connInfo)
  {
    return new EventInputConnector (connInfo,
				    spatialNegotiator,
				    handleEvent_,
				    type_,
				    setup_->communicator ());
  }

  
  EventHandlerGlobalIndexProxy*
  EventInputPort::allocEventHandlerGlobalIndexProxy (void (*eh) (double, int))
  {
    cEventHandlerGlobalIndex = EventHandlerGlobalIndexProxy (eh);
    return &cEventHandlerGlobalIndex;
  }

  
  EventHandlerLocalIndexProxy*
  EventInputPort::allocEventHandlerLocalIndexProxy (void (*eh) (double, int))
  {
    cEventHandlerLocalIndex = EventHandlerLocalIndexProxy (eh);
    return &cEventHandlerLocalIndex;
  }
  

  /********************************************************************
   *
   * Message Ports
   *
   ********************************************************************/

  MessagePort::MessagePort (Setup* s)
    : rank_ (s->communicator ().Get_rank ())
  {
  }
  
  
  MessageOutputPort::MessageOutputPort (Setup* s, std::string id)
    : Port (s, id), MessagePort (s)
  {
  }

  
  void
  MessageOutputPort::map ()
  {
    assertOutput ();
    int maxBuffered = MAX_BUFFERED_NO_VALUE;
    mapImpl (maxBuffered);
  }

  
  void
  MessageOutputPort::map (int maxBuffered)
  {
    assertOutput ();
    if (maxBuffered <= 0)
      {
	error ("MessageOutputPort::map: maxBuffered should be a positive integer");
      }
    mapImpl (maxBuffered);
  }

  
  void
  MessageOutputPort::mapImpl (int maxBuffered)
  {
    // Identify ourselves
    LinearIndex indices (rank_, 1);
    OutputRedistributionPort::mapImpl (&indices,
				       Index::GLOBAL,
				       maxBuffered,
				       1);
  }
  
  
  OutputConnector*
  MessageOutputPort::makeOutputConnector (ConnectorInfo connInfo)
  {
    return new MessageOutputConnector (connInfo,
				       spatialNegotiator,
				       setup_->communicator (),
				       buffers);
  }
  
  
  void
  MessageOutputPort::insertMessage (double t, void* msg, size_t size)
  {
    // One output buffer per OutputConnector (since different
    // connectors may need to send at different times)
    for (std::vector<FIBO*>::iterator b = buffers.begin ();
	 b != buffers.end ();
	 ++b)
      {
	MessageHeader header (t, size);
	(*b)->insert (header.data (), sizeof (MessageHeader));
	(*b)->insert (msg, size);
      }
  }

  
  MessageInputPort::MessageInputPort (Setup* s, std::string id)
    : Port (s, id), MessagePort (s)
  {
  }

  
  void
  MessageInputPort::map (MessageHandler* handleMessage,
			 double accLatency)
  {
    assertInput ();
    int maxBuffered = MAX_BUFFERED_NO_VALUE;
    mapImpl (handleMessage,
	     accLatency,
	     maxBuffered);
  }

  
  void
  MessageInputPort::map (int maxBuffered)
  {
    assertInput ();
    if (maxBuffered <= 0)
      {
	error ("MessageInputPort::map: maxBuffered should be a positive integer");
      }
    mapImpl (0,
	     0.0,
	     maxBuffered);
  }

  
  void
  MessageInputPort::map (double accLatency,
			 int maxBuffered)
  {
    assertInput ();
    if (maxBuffered <= 0)
      {
	error ("MessageInputPort::map: maxBuffered should be a positive integer");
      }
    mapImpl (0,
	     accLatency,
	     maxBuffered);
  }

  
  void
  MessageInputPort::map (MessageHandler* handleMessage,
			 int maxBuffered)
  {
    assertInput ();
    if (maxBuffered <= 0)
      {
	error ("MessageInputPort::map: maxBuffered should be a positive integer");
      }
    mapImpl (handleMessage,
	     0.0,
	     maxBuffered);
  }

  
  void
  MessageInputPort::map (MessageHandler* handleMessage,
			 double accLatency,
			 int maxBuffered)
  {
    assertInput ();
    if (maxBuffered <= 0)
      {
	error ("MessageInputPort::map: maxBuffered should be a positive integer");
      }
    mapImpl (handleMessage,
	     accLatency,
	     maxBuffered);
  }

  
  void
  MessageInputPort::mapImpl (MessageHandler* handleMessage,
			     double accLatency,
			     int maxBuffered)
  {
    handleMessage_ = handleMessage;
    // Receive from everybody
    LinearIndex indices (0, handleMessage ? Index::WILDCARD_MAX : 0);
    InputRedistributionPort::mapImpl (&indices,
				      Index::GLOBAL,
				      accLatency,
				      maxBuffered,
				      false);
  }

  
  InputConnector*
  MessageInputPort::makeInputConnector (ConnectorInfo connInfo)
  {
    return new MessageInputConnector (connInfo,
				      spatialNegotiator,
				      handleMessage_,
				      Index::GLOBAL,
				      setup_->communicator ());
  }

  
  MessageHandlerProxy*
  MessageInputPort::allocMessageHandlerProxy (void (*mh) (double,
								     void*,
								     size_t))
  {
    cMessageHandler = MessageHandlerProxy (mh);
    return &cMessageHandler;
  }

  
}

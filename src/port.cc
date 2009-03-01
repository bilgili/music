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

#include "music/setup.hh" // Must be included first on BG/L
#include "music/port.hh"
#include "music/error.hh"

namespace MUSIC {

  Port::Port (Setup* s, std::string identifier)
    : setup_ (s)
  {
    ConnectivityInfo_ = s->portConnectivity (identifier);
    setup_->addPort (this);
  }


  bool
  Port::isConnected ()
  {
    return ConnectivityInfo_ != Connectivity::NO_CONNECTIVITY;
  }


  void
  Port::assertOutput ()
  {
    if (!isConnected ())
      error ("attempt to map an unconnected port");
    else if (ConnectivityInfo_->direction () != ConnectivityInfo::OUTPUT)
      error ("output port connected as input");
  }


  void
  Port::assertInput ()
  {
    if (!isConnected ())
      error ("attempt to map an unconnected port");
    else if (ConnectivityInfo_->direction () != ConnectivityInfo::INPUT)
      error ("input port connected as output");
  }


  bool
  Port::hasWidth ()
  {
    if (!isConnected ())
      error ("attempt to ask for width of an unconnected port");
    return ConnectivityInfo_->width () != ConnectivityInfo::NO_WIDTH;
  }


  int
  Port::width ()
  {
    if (!isConnected ())
      error ("attempt to ask for width of an unconnected port");
    int w = ConnectivityInfo_->width ();
    if (w == ConnectivityInfo::NO_WIDTH)
      error ("width requested for port with unspecified width");
    return w;
  }


  void
  OutputRedistributionPort::setupCleanup ()
  {
    delete spatialNegotiator;
  }
  

  void
  OutputRedistributionPort::mapImpl (IndexMap* indices,
				     Index::Type type,
				     int maxBuffered,
				     int dataSize)
  {
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
	setup_->temporalNegotiator ()->addConnection (connector,
						      maxBuffered,
						      dataSize);
	setup_->addConnector (connector);
      }
  }


  void
  InputRedistributionPort::setupCleanup ()
  {
    delete spatialNegotiator;
  }

  void
  InputRedistributionPort::mapImpl (IndexMap* indices,
				    Index::Type type,
				    double accLatency,
				    int maxBuffered)
  {
    // Retrieve info about all remote connectors of this port
    PortConnectorInfo portConnections
      = ConnectivityInfo_->connections ();
    PortConnectorInfo::iterator info = portConnections.begin ();
    spatialNegotiator = new SpatialInputNegotiator (indices, type);
    InputConnector* connector = makeInputConnector (*info);
    setup_->temporalNegotiator ()->addConnection (connector,
						  maxBuffered,
						  accLatency);
    setup_->addConnector (connector);
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
				    sampler);
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
    assertInput ();
    interpolate_ = interpolate;
    sampler.configure (dmap);
    InputRedistributionPort::mapImpl (dmap->indexMap (),
				      Index::GLOBAL,
				      delay,
				      maxBuffered);
  }

  
  InputConnector*
  ContInputPort::makeInputConnector (ConnectorInfo connInfo)
  {
    return new ContInputConnector (connInfo,
				   spatialNegotiator,
				   setup_->communicator (),
				   sampler);
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
    InputRedistributionPort::mapImpl (indices, type, accLatency, maxBuffered);
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
				       router);
  }
  
  
  void
  MessageOutputPort::buildTable ()
  {
    router.buildTable ();
  }

  
  void
  MessageOutputPort::insertMessage (double t, void* msg, size_t size)
  {
    router.insertEvent (t, GlobalIndex (rank_));
  }

  
  MessageInputPort::MessageInputPort (Setup* s, std::string id)
    : Port (s, id), MessagePort (s)
  {
  }

  
  void
  MessageInputPort::map (MessageHandlerGlobalIndex* handleMessage,
			 double accLatency)
  {
    assertInput ();
    int maxBuffered = MAX_BUFFERED_NO_VALUE;
    mapImpl (MessageHandlerPtr (handleMessage),
	     accLatency,
	     maxBuffered);
  }

  
  void
  MessageInputPort::map (MessageHandlerGlobalIndex* handleMessage,
			 double accLatency,
			 int maxBuffered)
  {
    assertInput ();
    if (maxBuffered <= 0)
      {
	error ("MessageInputPort::map: maxBuffered should be a positive integer");
      }
    mapImpl (MessageHandlerPtr (handleMessage),
	     accLatency,
	     maxBuffered);
  }

  
  void
  MessageInputPort::mapImpl (MessageHandlerPtr handleMessage,
			     double accLatency,
			     int maxBuffered)
  {
    handleMessage_ = handleMessage;
    LinearIndex indices (rank_, 1);
    InputRedistributionPort::mapImpl (&indices,
				      Index::GLOBAL,
				      accLatency,
				      maxBuffered);
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

  
  MessageHandlerGlobalIndexProxy*
  MessageInputPort::allocMessageHandlerGlobalIndexProxy (void (*mh) (double,
								     void*,
								     size_t))
  {
    cMessageHandlerGlobalIndex = MessageHandlerGlobalIndexProxy (mh);
    return &cMessageHandlerGlobalIndex;
  }

  
}

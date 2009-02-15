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
  InputRedistributionPort::setupCleanup ()
  {
    delete spatialNegotiator;
  }
  
  
  void
  ContOutputPort::map (DataMap* dmap)
  {
    assertOutput ();
  }

  
  void
  ContOutputPort::map (DataMap* dmap, int maxBuffered)
  {
    assertOutput ();
  }

  
  void
  ContInputPort::map (DataMap* dmap, double delay, bool interpolate)
  {
    assertInput ();
  }

  
  void
  ContInputPort::map (DataMap* dmap,
		      int maxBuffered,
		      bool interpolate)
  {
    assertInput ();
  }

  
  void
  ContInputPort::map (DataMap* dmap,
		      double delay,
		      int maxBuffered,
		      bool interpolate)
  {
    assertInput ();
  }

  
  void
  EventOutputPort::map (IndexMap* indices, Index::Type type)
  {
    assertOutput ();
    int maxBuffered = MAX_BUFFERED_NO_VALUE;
    mapImpl (indices, type, maxBuffered);
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
    mapImpl (indices, type, maxBuffered);
  }

  
  void
  EventOutputPort::mapImpl (IndexMap* indices,
			    Index::Type type,
			    int maxBuffered)
  {
    MPI::Intracomm comm = setup_->communicator ();
    // Retrieve info about all remote connectors of this port
    PortConnectorInfo portConnections
      = ConnectivityInfo_->connections ();
    spatialNegotiator = new SpatialOutputNegotiator (indices, type);
    for (PortConnectorInfo::iterator info = portConnections.begin ();
	 info != portConnections.end ();
	 ++info)
      {
	// Create connector
	EventOutputConnector* connector
	  = new EventOutputConnector (*info,
				      spatialNegotiator,
				      comm,
				      router);
	setup_->temporalNegotiator ()->addConnection (connector,
						      maxBuffered,
						      sizeof (Event));
	setup_->addConnector (connector);
      }
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
    MPI::Intracomm comm = setup_->communicator ();
    // Retrieve info about all remote connectors of this port
    PortConnectorInfo portConnections
      = ConnectivityInfo_->connections ();
    PortConnectorInfo::iterator info = portConnections.begin ();
    spatialNegotiator = new SpatialInputNegotiator (indices, type);
    EventInputConnector* connector
      = new EventInputConnector (*info,
				 spatialNegotiator,
				 handleEvent,
				 type,
				 comm);
    setup_->temporalNegotiator ()->addConnection (connector,
						  maxBuffered,
						  accLatency);
    setup_->addConnector (connector);
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
  

  void
  MessageOutputPort::map ()
  {
    assertOutput ();
  }

  
  void
  MessageOutputPort::map (int maxBuffered)
  {
    assertOutput ();
  }

  
  void
  MessageInputPort::map (MessageHandler* handler, double accLatency)
  {
    assertOutput ();
  }

  
  void
  MessageInputPort::map (MessageHandler* handler,
			 double accLatency,
			 int maxBuffered)
  {
    assertOutput ();
  }

}

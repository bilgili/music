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

#include "music/setup.hh" // Must be included first on BG/L
#include "music/port.hh"
#include "music/error.hh"

namespace MUSIC {

  Port::Port (Setup* s, std::string identifier)
    : _setup (s)
  {
    _ConnectivityInfo = s->portConnectivity (identifier);
    _setup->addPort (this);
  }


  bool
  Port::isConnected ()
  {
    return _ConnectivityInfo != Connectivity::NO_CONNECTIVITY;
  }


  void
  Port::assertOutput ()
  {
    if (!isConnected ())
      error ("attempt to map an unconnected port");
    else if (_ConnectivityInfo->direction () != ConnectivityInfo::OUTPUT)
      error ("output port connected as input");
  }


  void
  Port::assertInput ()
  {
    if (!isConnected ())
      error ("attempt to map an unconnected port");
    else if (_ConnectivityInfo->direction () != ConnectivityInfo::INPUT)
      error ("input port connected as output");
  }


  bool
  Port::hasWidth ()
  {
    if (!isConnected ())
      error ("attempt to ask for width of an unconnected port");
    return _ConnectivityInfo->width () != ConnectivityInfo::NO_WIDTH;
  }


  int
  Port::width ()
  {
    if (!isConnected ())
      error ("attempt to ask for width of an unconnected port");
    int w = _ConnectivityInfo->width ();
    if (w == ConnectivityInfo::NO_WIDTH)
      error ("width requested for port with unspecified width");
    return w;
  }


  void
  OutputRedistributionPort::setupCleanup ()
  {
    delete negotiator;
  }
  
  
  void
  InputRedistributionPort::setupCleanup ()
  {
    delete negotiator;
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
    int maxBuffered = 0;
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
    MPI::Intracomm comm = _setup->communicator ();
    // Retrieve info about all remote connectors of this port
    PortConnectorInfo portConnections
      = _ConnectivityInfo->connections ();
    negotiator = new SpatialOutputNegotiator (indices, type);
    for (PortConnectorInfo::iterator info = portConnections.begin ();
	 info != portConnections.end ();
	 ++info)
      // Create connector
      _setup->addConnector (new EventOutputConnector (*info,
							 negotiator,
							 maxBuffered,
							 comm,
							 router));
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
    int maxBuffered = 0;
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
    int maxBuffered = 0;
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
    MPI::Intracomm comm = _setup->communicator ();
    // Retrieve info about all remote connectors of this port
    PortConnectorInfo portConnections
      = _ConnectivityInfo->connections ();
    PortConnectorInfo::iterator info = portConnections.begin ();
    negotiator = new SpatialInputNegotiator (indices, type);
    _setup->addConnector (new EventInputConnector (*info,
						      negotiator,
						      handleEvent,
						      type,
						      accLatency,
						      maxBuffered,
						      comm));
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

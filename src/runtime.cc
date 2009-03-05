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

//#define MUSIC_DEBUG 1
#include "music/debug.hh"

#include <mpi.h>

#include <algorithm>

#include "music/runtime.hh"
#include "music/temporal.hh"

namespace MUSIC {

  Runtime::Runtime (Setup* s, double h)
  {
    OutputSubconnectors outputSubconnectors;
    InputSubconnectors inputSubconnectors;
    
    // Setup the MUSIC clock
    localTime = Clock (s->timebase (), h);
    
    comm = s->communicator ();

    Connections* connections = s->connections ();
    
    if (s->launchedByMusic ())
      {
	takeTickingPorts (s);
	
	// create a total order for connectors and
	// establish connection to peers
	connectToPeers (connections);
	
	// specialize connectors and fill up connectors vector
	specializeConnectors (connections);
	
	// from here we can start using the vector `connectors'

	// negotiate where to route data and fill up subconnector vectors
	spatialNegotiation (outputSubconnectors, inputSubconnectors);

	// build data routing tables
	buildTables (s);

	// build a total order of subconnectors
	// for non-blocking pairwise exchange
	buildSchedule (comm.Get_rank (),
		       outputSubconnectors,
		       inputSubconnectors);
	
	takePostCommunicators ();
	
	// negotiate timing constraints for synchronizers
	temporalNegotiation (s, connections);
	
	// final initialization before simulation starts
	initialize ();
      }
    
    delete s;
  }


  Runtime::~Runtime ()
  {
    // delete subconnectors
    for (std::vector<Subconnector*>::iterator subconnector = schedule.begin ();
	 subconnector != schedule.end ();
	 ++subconnector)
      delete *subconnector;

#if 0
    // delete connectors
    for (std::vector<Connector*>::iterator connector = connectors.begin ();
	 connector != connectors.end ();
	 ++connector)
      delete *connector;
#endif
  }
  

  void
  Runtime::takeTickingPorts (Setup* s)
  {
    std::vector<Port*>::iterator p;
    for (p = s->ports ()->begin (); p != s->ports ()->end (); ++p)
      {
	TickingPort* tp = dynamic_cast<TickingPort*> (*p);
	if (tp != NULL)
	  tickingPorts.push_back (tp);
      }
  }
  

  void
  Runtime::takePostCommunicators ()
  {
    std::vector<Connector*>::iterator c;
    for (c = connectors.begin (); c != connectors.end (); ++c)
      {
	PostCommunicationConnector* postCommunicationConnector
	  = dynamic_cast<PostCommunicationConnector*> (*c);
	if (postCommunicationConnector != NULL)
	  postCommunication.push_back (postCommunicationConnector);
      }
  }
  

  // This predicate gives a total order for connectors which is the
  // same on the sender and receiver sides.  It belongs here rather
  // than in connector.hh or connector.cc since it is connected to the
  // connect algorithm.
  bool
  lessConnection (const Connection* cn1, const Connection* cn2)
  {
    Connector* c1 = cn1->connector ();
    Connector* c2 = cn2->connector ();
    return (c1->receiverAppName () < c2->receiverAppName ()
	    || (c1->receiverAppName () == c2->receiverAppName ()
		&& c1->receiverPortName () < c2->receiverPortName ()));
  }
  
  
  // This predicate gives a total order for subconnectors which is the
  // same on the sender and receiver sides.  It belongs here rather
  // than in connector.hh or connector.cc since it is connected to the
  // connect algorithm.
  bool
  lessSubconnector (const Subconnector* c1, const Subconnector* c2)
  {
    return (c1->receiverRank () < c2->receiverRank ()
	    || (c1->receiverRank () == c2->receiverRank ()
		&& c1->receiverPortName () < c2->receiverPortName ()));
  }

  
  void
  Runtime::connectToPeers (Connections* connections)
  {
    // This ordering is necessary so that both sender and receiver
    // in each pair sets up communication at the same point in time
    //
    // Note that we don't need to use the dead-lock scheme used in
    // build_schedule () here.
    //
    sort (connections->begin (), connections->end (), lessConnection);
    for (Connections::iterator c = connections->begin ();
	 c != connections->end ();
	 ++c)
      (*c)->connector ()->createIntercomm ();
  }


  void
  Runtime::specializeConnectors (Connections* connections)
  {
    for (Connections::iterator c = connections->begin ();
	 c != connections->end ();
	 ++c)
      {
	Connector* oldConnector = (*c)->connector ();
	Connector* newConnector = oldConnector->specialize (localTime);
	//delete oldConnector;
	
	(*c)->setConnector (newConnector);
	connectors.push_back (newConnector);
      }
  }

  
  void
  Runtime::spatialNegotiation (OutputSubconnectors& outputSubconnectors,
			       InputSubconnectors& inputSubconnectors)
  {
    // Let each connector pair setup their inter-communicators
    // and create all required subconnectors.

    for (std::vector<Connector*>::iterator c = connectors.begin ();
	 c != connectors.end ();
	 ++c)
      {
	// negotiate and fill up vectors passed as arguments
	(*c)->spatialNegotiation (outputSubconnectors, inputSubconnectors);
      }
  }


  void
  Runtime::buildSchedule (int localRank,
			  OutputSubconnectors& outputSubconnectors,
			  InputSubconnectors& inputSubconnectors)
  {
    // Build the communication schedule.
    
    // The following communication schedule prevents dead-locking,
    // both during inter-communicator creation and during
    // communication of data.
    //
    // Mikael Djurfeldt et al. (2005) "Massively parallel simulation
    // of brain-scale neuronal network models." Tech. Rep.
    // TRITA-NA-P0513, CSC, KTH, Stockholm.
    
    // First sort connectors according to a total order
    
    sort (outputSubconnectors.begin (), outputSubconnectors.end (),
	  lessSubconnector);
    sort (inputSubconnectors.begin (), inputSubconnectors.end (),
	  lessSubconnector);

    // Now, build up the schedule to be used later during
    // communication of data.
    for (std::vector<InputSubconnector*>::iterator c = inputSubconnectors.begin ();
	 (c != inputSubconnectors.end ()
	  && (*c)->remoteRank () < localRank);
	 ++c)
      schedule.push_back (*c);
    {
      std::vector<OutputSubconnector*>::iterator c = outputSubconnectors.begin ();
      for (;
	   (c != outputSubconnectors.end ()
	    && (*c)->remoteRank () < localRank);
	   ++c)
	;
      for (; c != outputSubconnectors.end (); ++c)
	schedule.push_back (*c);
    }
    for (std::vector<OutputSubconnector*>::iterator c = outputSubconnectors.begin ();
	 (c != outputSubconnectors.end ()
	  && (*c)->remoteRank () < localRank);
	 ++c)
      schedule.push_back (*c);
    {
      std::vector<InputSubconnector*>::iterator c = inputSubconnectors.begin ();
      for (;
	   (c != inputSubconnectors.end ()
	    && (*c)->remoteRank () < localRank);
	   ++c)
	;
      for (; c != inputSubconnectors.end (); ++c)
	schedule.push_back (*c);
    }

  }

  
  void
  Runtime::buildTables (Setup* s)
  {
    std::vector<Port*>* ports = s->ports ();
    for (std::vector<Port*>::iterator p = ports->begin ();
	 p != ports->end ();
	 ++p)
      (*p)->buildTable ();
  }

  
  void
  Runtime::temporalNegotiation (Setup* s, Connections* connections)
  {
    // Temporal negotiation is done globally by a serial algorithm
    // which yields the same result in each process
    s->temporalNegotiator ()->negotiate (localTime, connections);
  }


  MPI::Intracomm
  Runtime::communicator ()
  {
    return comm;
  }


  void
  Runtime::initialize ()
  {
    // clocks are already set by temporal negotiation to -maxDelay (in
    // whole ticks so that all clocks will pass time zero)

    // initialize connectors (and synchronizers)
    std::vector<Connector*>::iterator c;
    for (c = connectors.begin (); c != connectors.end (); ++c)
      (*c)->initialize ();

    while (localTime.integerTime () < 0)
      tick ();
  }

  
  void
  Runtime::finalize ()
  {
    bool dataStillFlowing;
    do
      {
	dataStillFlowing = false;
	std::vector<Subconnector*>::iterator c;
	for (c = schedule.begin (); c != schedule.end (); ++c)
	  (*c)->flush (dataStillFlowing);
      }
    while (dataStillFlowing);
    
    MPI::Finalize ();
  }


  void
  Runtime::tick ()
  {
    // Update local time
    localTime.tick ();
    
    // ContPorts do some per-tick initialization here
    std::vector<TickingPort*>::iterator p;
    for (p = tickingPorts.begin (); p != tickingPorts.end (); ++p)
      (*p)->tick ();

    // Check if any connector wants to communicate
    bool requestCommunication = false;

    std::vector<Connector*>::iterator c;
    for (c = connectors.begin (); c != connectors.end (); ++c)
      (*c)->tick (requestCommunication);

    // Communicate data through non-interlocking pair-wise exchange
    if (requestCommunication)
      {
	// Loop through the schedule of subconnectors
	for (std::vector<Subconnector*>::iterator s = schedule.begin ();
	     s != schedule.end ();
	     ++s)
	  (*s)->maybeCommunicate ();
      }

    // ContInputConnectors write data to application here
    for (std::vector<PostCommunicationConnector*>::iterator c
	   = postCommunication.begin ();
	 c != postCommunication.end ();
	 ++c)
      (*c)->postCommunication ();
	
  }


  double
  Runtime::time ()
  {
    return localTime.time ();
  }
  
}

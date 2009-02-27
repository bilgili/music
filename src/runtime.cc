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
    // Setup the MUSIC clock
    localTime = Clock (s->timebase (), h);
    
    comm = s->communicator ();
    // Take over connectors from setup object
    connectors = s->connectors ();
    if (s->launchedByMusic ())
      {
	takeTickingPorts (s);
	connectToPeers ();
	specializeConnectors ();
	MUSIC_LOG ("spatial negotiation");
	spatialNegotiation ();
	buildTables (s);
	buildSchedule (comm.Get_rank ());
	takePostCommunicators ();
	MUSIC_LOG ("temporal negotiation");
	temporalNegotiation (s, localTime);
	// final initialization before simulation starts
	initialize ();
      }
    delete s;
  }


  Runtime::~Runtime ()
  {
    for (std::vector<OutputSubconnector*>::iterator c
	   = outputSubconnectors.begin ();
	 c != outputSubconnectors.end ();
	 ++c)
      delete *c;
    for (std::vector<InputSubconnector*>::iterator c
	   = inputSubconnectors.begin ();
	 c != inputSubconnectors.end ();
	 ++c)
      delete *c;
    delete connectors;
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
    for (c = connectors->begin (); c != connectors->end (); ++c)
      {
	PostCommunicationConnector* pc
	  = dynamic_cast<PostCommunicationConnector*> (*c);
	if (pc != NULL)
	  postCommunication.push_back (pc);
      }
  }
  

  // This predicate gives a total order for connectors which is the
  // same on the sender and receiver sides.  It belongs here rather
  // than in connector.hh or connector.cc since it is connected to the
  // connect algorithm.
  bool
  lessConnector (const Connector* c1, const Connector* c2)
  {
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
  Runtime::connectToPeers ()
  {
    // This ordering is necessary so that both sender and receiver
    // in each pair sets up communication at the same point in time
    //
    // Note that we don't need to use the dead-lock scheme used in
    // build_schedule () here.
    //
    sort (connectors->begin (), connectors->end (), lessConnector);
    for (std::vector<Connector*>::iterator c = connectors->begin ();
	 c != connectors->end ();
	 ++c)
      (*c)->createIntercomm ();
  }

  
  void
  Runtime::specializeConnectors ()
  {
    for (std::vector<Connector*>::iterator c = connectors->begin ();
	 c != connectors->end ();
	 ++c)
      *c = (*c)->specialize (localTime.tickInterval ());
  }

  
  void
  Runtime::spatialNegotiation ()
  {
    // Let each connector pair setup their inter-communicators
    // and create all required subconnectors.

    sort (connectors->begin (), connectors->end (), lessConnector);
    for (std::vector<Connector*>::iterator c = connectors->begin ();
	 c != connectors->end ();
	 ++c)
      // negotiate and fill up vectors passed as arguments
      (*c)->spatialNegotiation (outputSubconnectors, inputSubconnectors);
  }


  void
  Runtime::buildSchedule (int localRank)
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

    //*fixme* could delete output/inputSubconnectors here
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
  Runtime::temporalNegotiation (Setup* s, Clock& localTime)
  {
    // Temporal negotiation is done globally by a serial algorithm
    // which yields the same result in each process
    s->temporalNegotiator ()->negotiate (localTime);
  }


  MPI::Intracomm
  Runtime::communicator ()
  {
    return comm;
  }


  void
  Runtime::initialize ()
  {
    std::vector<Connector*>::iterator c;
    for (c = connectors->begin (); c != connectors->end (); ++c)
      (*c)->initialize ();
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
    // ContPorts do some per-tick initialization here
    std::vector<TickingPort*>::iterator p;
    for (p = tickingPorts.begin (); p != tickingPorts.end (); ++p)
      (*p)->tick ();

    // Check if any connector wants to communicate
    bool requestCommunication = false;

    std::vector<Connector*>::iterator c;
    for (c = connectors->begin (); c != connectors->end (); ++c)
      (*c)->tick (requestCommunication);

    // Communicate data through non-interlocking pair-wise exchange
    if (requestCommunication)
      {
	// Loop through the schedule of subconnectors
	for (std::vector<Subconnector*>::iterator s = schedule.begin ();
	     s != schedule.end ();
	     ++s)
	  (*s)->tick ();//*fixme* rename subconnector tick to maybeCommunicate?
      }

    // ContInputConnectors write data to application here
    for (std::vector<PostCommunicationConnector*>::iterator c
	   = postCommunication.begin ();
	 c != postCommunication.end ();
	 ++c)
      (*c)->postCommunication ();
	
    // Update local time
    localTime.tick ();
  }


  double
  Runtime::time ()
  {
    return localTime.time ();
  }
  
}

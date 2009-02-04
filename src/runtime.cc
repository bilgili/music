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
    double timebase;
    if (!s->config ("timebase", &timebase))
      timebase = 1e-9;		// default timebase
    localTime = Clock (timebase, h);
    
    comm = s->communicator ();
    if (s->launchedByMusic ())
      {
	// Take over connectors from setup object
	connectors = s->connectors ();
	MUSIC_LOG ("spatial negotiation");
	spatialNegotiation (s);
	buildTables (s);
	buildSchedule (comm.Get_rank ());
	MUSIC_LOG ("temporal negotiation");
	temporalNegotiation (s, localTime);
#if 0
	rank = MPI::COMM_WORLD.Get_rank ();
	Connector* conn = *connectors->begin ();
	int latency, maxBuffered;
	if (rank == 0)
	  {
	    OutputConnector* oconn = dynamic_cast<OutputConnector*> (conn);
	    OutputSynchronizer* synch = oconn->synchronizer ();
	    latency = synch->latency;
	    maxBuffered = synch->maxBuffered;
	    sendClock = &synch->nextSend;
	    receiveClock = &synch->nextReceive;
	    MUSIC_LOG ("rank " << rank
		       << ": latency = " << latency
		       << ", maxBuffered = " << maxBuffered);
	  }
	else
	  {
	    InputConnector* oconn = dynamic_cast<InputConnector*> (conn);
	    InputSynchronizer* synch = oconn->synchronizer ();
	    latency = synch->latency;
	    maxBuffered = synch->maxBuffered;
	    sendClock = &synch->nextSend;
	    receiveClock = &synch->nextReceive;
	    MUSIC_LOG ("rank " << rank
		       << ": latency = " << latency
		       << ", maxBuffered = " << maxBuffered);
	  }
#endif
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
  Runtime::spatialNegotiation (Setup* s)
  {
    // Let each connector pair setup their inter-communicators
    // and create all required subconnectors.

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
  Runtime::finalize ()
  {
    MPI::Finalize ();
  }


  void
  Runtime::tick ()
  {
    MUSIC_LOG ("rank " << rank
	       << ": time = " << localTime.time ()
	       << ", nextSend = " << sendClock->time ()
	       << ", nextReceive = " << receiveClock->time ());
    bool requestCommunication = false;
    
    std::vector<Connector*>::iterator c;
    for (c = connectors->begin (); c != connectors->end (); ++c)
      (*c)->tick (requestCommunication);

    if (requestCommunication)
      {
	// Loop through the schedule of subconnectors
	std::vector<Subconnector*>::iterator c;
	for (c = schedule.begin (); c != schedule.end (); ++c)
	  (*c)->tick ();//*fixme* rename subconnector tick to doCommunicate?
      }
    
    localTime.tick ();
  }


  double
  Runtime::time ()
  {
    return localTime.time ();
  }
  
}

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
#include "music/runtime.hh"
#ifdef USE_MPI

#include <mpi.h>

#include <algorithm>
#include <iostream>

#include "music/temporal.hh"
#include "music/error.hh"

namespace MUSIC {

  bool Runtime::isInstantiated_ = false;

  Runtime::Runtime (Setup* s, double h)
  {
    checkInstantiatedOnce (isInstantiated_, "Runtime");
    
    OutputSubconnectors outputSubconnectors;
    InputSubconnectors inputSubconnectors;
    /* remedius
     * new type of subconnectors for collective communication was created.
     */
    CollectiveSubconnectors collectiveSubconnectors;
    // Setup the MUSIC clock
    localTime = Clock (s->timebase (), h);
    
    comm = s->communicator ();

    /* remedius
     * copy ports in order to delete them afterwards (it was memory leak)
     */
    for ( std::vector<Port *>::iterator it=s->ports()->begin() ; it < s->ports()->end(); it++ )
    	ports.push_back((*it));

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

	spatialNegotiation (outputSubconnectors, inputSubconnectors, collectiveSubconnectors);
	// build data routing tables
	buildTables (s);
	// build a total order of subconnectors
	// for non-blocking pairwise exchange
	buildSchedule (
		       outputSubconnectors,
		       inputSubconnectors,
		       collectiveSubconnectors);
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

    // delete connectors
    for (std::vector<Connector*>::iterator connector = connectors.begin ();
	 connector != connectors.end ();
	 ++connector)
      delete *connector;

    // delete ports
    for (std::vector<Port *>::iterator it=ports.begin() ; it < ports.end(); it++ )
        delete (*it);
    isInstantiated_ = false;
  }
  

  void
  Runtime::takeTickingPorts (Setup* s)
  {
    std::vector<Port*>::iterator p;
    for (p = ports.begin (); p != ports.end (); ++p)
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
    return (c1->remoteWorldRank () < c2->remoteWorldRank ()
	    || (c1->remoteWorldRank () == c2->remoteWorldRank ()
		&& c1->receiverPortCode () < c2->receiverPortCode ()));
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
	Connector* connector = (*c)->connector ()->specialize (localTime);
	(*c)->setConnector (connector);
	connectors.push_back (connector);
      }
  }


  void
  Runtime::spatialNegotiation (OutputSubconnectors& outputSubconnectors,
			       InputSubconnectors& inputSubconnectors, CollectiveSubconnectors& collectiveSubconnectors)
  {
    // Let each connector pair setup their inter-communicators
    // and create all required subconnectors.

    int type;
    for (std::vector<Connector*>::iterator c = connectors.begin ();
	 c != connectors.end ();
	 ++c)
      {
    	Subconnectors subconnectors;
	   // negotiate and fill up a vector passed as an argument
    	type = (*c)->spatialNegotiation (subconnectors);

        /* remedius
         * cast subconnectors to an appropriate type.
         */
    	int rsize = subconnectors.size();
    	if(type == OUTPUT_SUBCONNECTORS){
    		outputSubconnectors.resize(outputSubconnectors.size()+rsize);
    		transform( subconnectors.begin(), subconnectors.end(),
    				outputSubconnectors.end()-rsize,
    				Subconnector2Target<OutputSubconnector>() );


    	}
    	else if( type == INPUT_SUBCONNECTORS){
    		inputSubconnectors.resize(inputSubconnectors.size()+rsize);
    		transform( subconnectors.begin(), subconnectors.end(),
    				inputSubconnectors.end()-rsize,
    				Subconnector2Target<InputSubconnector>() );
    	}
    	else if(type == COLLECTIVE_SUBCONNECTORS){
    		collectiveSubconnectors.resize(collectiveSubconnectors.size()+rsize);
    		transform( subconnectors.begin(), subconnectors.end(),
    				collectiveSubconnectors.end()-rsize,
    				Subconnector2Target<CollectiveSubconnector>() );
    	}
    	else{

    		error0(" Runtime::spatialNegotiation::undefined subconnector");
    	}
      }

  }


  void
  Runtime::buildSchedule (
			  OutputSubconnectors& outputSubconnectors,
			  InputSubconnectors& inputSubconnectors,
			  CollectiveSubconnectors& collectiveSubconnectors)
  {
	  int localRank = MPI::COMM_WORLD.Get_rank ();
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
	  && (*c)->remoteWorldRank () < localRank);
	 ++c)
      schedule.push_back (*c);
    {
      std::vector<OutputSubconnector*>::iterator c = outputSubconnectors.begin ();
      for (;
	   (c != outputSubconnectors.end ()
	    && (*c)->remoteWorldRank () < localRank);
	   ++c)
	;
      for (; c != outputSubconnectors.end (); ++c)
	schedule.push_back (*c);
    }
    for (std::vector<OutputSubconnector*>::iterator c = outputSubconnectors.begin ();
	 (c != outputSubconnectors.end ()
	  && (*c)->remoteWorldRank () < localRank);
	 ++c)
      schedule.push_back (*c);
    {
      std::vector<InputSubconnector*>::iterator c = inputSubconnectors.begin ();
      for (;
	   (c != inputSubconnectors.end ()
	    && (*c)->remoteWorldRank () < localRank);
	   ++c)
	;
      for (; c != inputSubconnectors.end (); ++c)
	schedule.push_back (*c);

    }

    for (std::vector<CollectiveSubconnector*>::iterator c = collectiveSubconnectors.begin ();
         	 c != collectiveSubconnectors.end ();	 ++c)
               schedule.push_back (*c);
  }

  
  void
  Runtime::buildTables (Setup* s)
  {
    for (std::vector<Port*>::iterator p = ports.begin ();
	 p != ports.end ();
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
    // initialize connectors (and synchronizers)
    std::vector<Connector*>::iterator c;
    for (c = connectors.begin (); c != connectors.end (); ++c)
      (*c)->initialize ();

    // receive first chunk of data from sender application and fill
    // cont buffers according to Synchronizer::initialBufferedTicks ()
    for (std::vector<Subconnector*>::iterator s = schedule.begin ();
	 s != schedule.end ();
	 ++s)
      (*s)->initialCommunication ();
    for (c = connectors.begin (); c != connectors.end (); ++c)
      (*c)->prepareForSimulation ();

    // compensate for first localTime.tick () in Runtime::tick ()
    localTime.ticks (-1);

    // the time zero tick () (where we may or may not communicate)
    tick ();
  }

  
  void
  Runtime::finalize ()
  {
    bool dataStillFlowing;
    do
      {
	std::vector<Subconnector*>::iterator c;
	 dataStillFlowing = false;
	for (c = schedule.begin (); c != schedule.end (); ++c)
	  (*c)->flush (dataStillFlowing);
      }
    while (dataStillFlowing);
#if defined (OPEN_MPI) && MPI_VERSION <= 2
    // This is needed in OpenMPI version <= 1.2 for the freeing of the
    // intercommunicators to go well
    MPI::COMM_WORLD.Barrier ();
#endif
    for (std::vector<Connector*>::iterator connector = connectors.begin ();
	 connector != connectors.end ();
	 ++connector)
      (*connector)->freeIntercomm ();
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
#endif

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
#include <set>
#include "music/temporal.hh"
#include "music/error.hh"
#include "music/connection.hh"
namespace MUSIC {

bool Runtime::isInstantiated_ = false;

Runtime::Runtime (Setup* s, double h)
{
	checkInstantiatedOnce (isInstantiated_, "Runtime");


	ApplicationMap* applicationMap = s->applicationMap ();
	int local_node = rankToNode(applicationMap);
	scheduler = new Scheduler(local_node);
	app_name= (*applicationMap)[local_node].name();
	/* remedius
	 * new type of subconnectors for collective communication was created.
	 */
	//CollectiveSubconnectors collectiveSubconnectors;
	// Setup the MUSIC clock
	localTime = Clock (s->timebase (), h);

	comm = s->communicator ();

	/* remedius
	 * copy ports in order to delete them afterwards (it was probably a memory leak in previous revision)
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
		spatialNegotiation ();
		// build data routing tables
		buildTables (s);

		takePreCommunicators ();
		takePostCommunicators ();

		// negotiate timing constraints for synchronizers
		temporalNegotiation (s, scheduler, connections);
		// final initialization before simulation starts
		initialize ();
	}

	delete s;
}


Runtime::~Runtime ()
{
	// delete connectors
	for (std::vector<Connector*>::iterator connector = connectors.begin ();
			connector != connectors.end ();
			++connector)
		delete *connector;

	// delete ports
	for (std::vector<Port *>::iterator it=ports.begin() ; it < ports.end(); it++ )
		delete (*it);
	delete scheduler;
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
Runtime::takePreCommunicators ()
{
	std::vector<Connector*>::iterator c;
	for (c = connectors.begin (); c != connectors.end (); ++c)
	{
		PreCommunicationConnector* preCommunicationConnector
		= dynamic_cast<PreCommunicationConnector*> (*c);
		if (preCommunicationConnector != NULL)
			preCommunication.push_back (preCommunicationConnector);
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
Runtime::spatialNegotiation ()
{
	// Let each connector pair setup their inter-communicators
	// and create all required subconnectors.

	for (std::vector<Connector*>::iterator c = connectors.begin ();
			c != connectors.end ();
			++c)
	{
		//Subconnectors subconnectors;
		// negotiate and fill up a vector passed as an argument
		(*c)->spatialNegotiation ();

	}

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
Runtime::temporalNegotiation (Setup* s, Scheduler *scheduler, Connections* connections)
{
	// Temporal negotiation is done globally by a serial algorithm
	// which yields the same result in each process
	s->temporalNegotiator ()->negotiate (localTime, connections, scheduler);
}

MPI::Intracomm
Runtime::communicator ()
{
	return comm;
}


void
Runtime::initialize ()
{

	/* renedius
	 * connector's initialization was moved to scheduler->initialize()
	 */
	scheduler->initialize(connectors);
	scheduler->nextCommunication(nextComm, schedule);

	// compensate for first localTime.tick () in Runtime::tick ()
	localTime.ticks (-1);
	// the time zero tick () (where we may or may not communicate)
	tick ();
}


void
Runtime::finalize ()
{
	/* remedius
	 * set of receiver port codes that still has to be finalized
	 */
	std::set<int> cnn_ports;
	for (std::vector<Connector*>::iterator c = connectors.begin (); c != connectors.end (); ++c)
		cnn_ports.insert((*c)->receiverPortCode());
	/* remedius
	 * finalize communication
	 */
	do
	{
		while (!schedule.empty())
		{
			if(schedule.front()->finalizeSimulation())
				cnn_ports.erase(schedule.front()->receiverPortCode());
			schedule.pop();
		}
		scheduler->nextCommunication(nextComm, schedule);
	}
	while (!cnn_ports.empty());
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
	localTime.tick();
	// ContPorts do some per-tick initialization here
	std::vector<TickingPort*>::iterator p;
	for (p = tickingPorts.begin(); p != tickingPorts.end(); ++p)
		(*p)->tick();
	// ContOutputConnectors sample data
	for (std::vector<PreCommunicationConnector*>::iterator c =
			preCommunication.begin(); c != preCommunication.end(); ++c)
		(*c)->preCommunication();
	MUSIC_LOG0("local time:" << localTime.time() << "next communication at (" << nextComm.time() << ")");
	while (nextComm.time() <= localTime.time()) { // should be ==
		while (!schedule.empty()) {
			schedule.front()->tick();
			schedule.pop();
		}
		scheduler->nextCommunication(nextComm, schedule);
	}
	// ContInputConnectors write data to application here
	for (std::vector<PostCommunicationConnector*>::iterator c =
			postCommunication.begin(); c != postCommunication.end(); ++c)
		(*c)->postCommunication();
}

double
Runtime::time ()
{
	return localTime.time ();
}
int Runtime::rankToNode(ApplicationMap* applicationMap){
	int local_node = -1;
	int rank = MPI::COMM_WORLD.Get_rank ();
	int nApplications = applicationMap->size();
	for (int i = 0; i < nApplications; ++i)
		if(rank >= (*applicationMap)[i].leader())
			local_node = i;
	return local_node;
}
}
#endif

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
#include "music/debug.hh" // Must be included first on BG/L

#include "music/setup.hh"
#include "music/temporal.hh"
#include "music/error.hh"

namespace MUSIC {

  TemporalNegotiator::TemporalNegotiator (Setup* setup)
    : setup_ (setup)
  {
  }


  TemporalNegotiator::~TemporalNegotiator ()
  {
    freeNegotiationData (negotiationBuffer);
    negotiationComm.Free ();
  }


  void
  TemporalNegotiator::addConnection (OutputConnector* connector,
				     int maxBuffered,
				     int elementSize)
  {
    outputConnections.push_back (OutputConnection (connector,
						   maxBuffered,
						   elementSize));
  }
  
  void
  TemporalNegotiator::addConnection (InputConnector* connector,
				     int maxBuffered,
				     double accLatency)
  {
    ClockStateT integerLatency = accLatency / setup_->timebase () + 0.5;
    inputConnections.push_back (InputConnection (connector,
						 maxBuffered,
						 integerLatency));
  }


  bool
  TemporalNegotiator::isLeader ()
  {
    return setup_->communicator ().Get_rank () == 0;
  }


  bool
  TemporalNegotiator::hasPeers ()
  {
    return setup_->communicator ().Get_size () > 1;
  }


  void
  TemporalNegotiator::createNegotiationCommunicator ()
  {
    ApplicationMap* applicationMap = setup_->applicationMap ();
    nApplications = applicationMap->size ();
    int* ranks = new int[nApplications];
    int localRank = MPI::COMM_WORLD.Get_rank ();
    for (int i = 0; i < nApplications; ++i)
      {
	int leader = (*applicationMap)[i].leader ();
	ranks[i] = leader;
	leaderToNode.insert (std::make_pair (leader, i));
	if (leader == localRank)
	  localNode = i;
      }
    MPI::Group groupWorld = MPI::COMM_WORLD.Get_group ();
    MPI::Group applicationLeaders = groupWorld.Incl (nApplications, ranks);
    delete[] ranks;
    negotiationComm = MPI::COMM_WORLD.Create (applicationLeaders);
    applicationLeaders.Free ();
  }


  int
  TemporalNegotiator::negotiationDataSize (int nConnections)
  {
    return negotiationDataSize (1, nConnections);
  }


  int
  TemporalNegotiator::negotiationDataSize (int nBlocks, int nConnections)
  {
    return (nBlocks * sizeof (TemporalNegotiationData)
	    + nConnections * sizeof (ConnectionDescriptor));
  }


  TemporalNegotiationData*
  TemporalNegotiator::allocNegotiationData (int nBlocks, int nConnections)
  {
    void* memory = new char[negotiationDataSize (nBlocks, nConnections)];
    return static_cast<TemporalNegotiationData*> (memory);
  }


  void
  TemporalNegotiator::freeNegotiationData (TemporalNegotiationData* data)
  {
    delete[] static_cast<char*> (static_cast<void*> (data));
  }


  int
  TemporalNegotiator::computeDefaultMaxBuffered (int maxLocalWidth,
						 int eventSize,
						 ClockStateT tickInterval,
						 double timebase)
  {
    int res;
    if (eventSize == 0)
      // continuous data
      res = DEFAULT_PACKET_SIZE / maxLocalWidth;
    else
      // event data
      res = (DEFAULT_PACKET_SIZE
	     / (EVENT_FREQUENCY_ESTIMATE
		* maxLocalWidth * eventSize * timebase * tickInterval));
    if (res < 1)
      res = 1;
    return res;
  }

  
  void
  TemporalNegotiator::collectNegotiationData (ClockStateT ti)
  {
    int nOut = outputConnections.size ();
    int nIn = inputConnections.size ();
    nLocalConnections = nOut + nIn;
    MUSIC_LOG ("nLocalConnections = " << nLocalConnections
	       << ", nOut = " << nOut
	       << ", nIn = " << nIn);
    negotiationData = allocNegotiationData (1, nLocalConnections);
    negotiationData->timeBase = setup_->timebase ();
    negotiationData->tickInterval = ti;
    negotiationData->nOutConnections = outputConnections.size ();
    negotiationData->nInConnections = inputConnections.size ();
    for (int i = 0; i < nOut; ++i)
      {
	OutputConnector* connector = outputConnections[i].connector ();
	int remote = connector->remoteLeader ();
	negotiationData->connection[i].remoteNode = leaderToNode[remote];
	negotiationData->connection[i].receiverPort
	  = connector->receiverPortCode ();
	negotiationData->connection[i].maxBuffered
	  = outputConnections[i].maxBuffered ();
	negotiationData->connection[i].defaultMaxBuffered
	  = computeDefaultMaxBuffered (connector->maxLocalWidth (),
				       outputConnections[i].elementSize (),
				       ti,
				       setup_->timebase ());
	negotiationData->connection[i].accLatency = 0;
      }
    for (int i = 0; i < nIn; ++i)
      {
	int remote = inputConnections[i].connector ()->remoteLeader ();
	negotiationData->connection[nOut + i].remoteNode = leaderToNode[remote];
	negotiationData->connection[nOut + i].receiverPort
	  = inputConnections[i].connector ()->receiverPortCode ();
	negotiationData->connection[nOut + i].maxBuffered
	  = inputConnections[i].maxBuffered ();
	negotiationData->connection[nOut + i].defaultMaxBuffered = 0;
	negotiationData->connection[nOut + i].accLatency
	  = inputConnections[i].accLatency ();
      }
  }


  void
  TemporalNegotiator::communicateNegotiationData ()
  {
    // First talk to others about how many connections each node has
    int* nConnections = new int[nApplications];
    negotiationComm.Allgather (&nLocalConnections, 1, MPI::INT,
			       nConnections, 1, MPI::INT);
    int nAllConnections = 0;
    for (int i = 0; i < nApplications; ++i)
      nAllConnections += nConnections[i];
    negotiationBuffer = allocNegotiationData (nApplications, nAllConnections);

    char* memory = static_cast<char*> (static_cast<void*> (negotiationBuffer));
    int* receiveSizes = new int[nApplications];
    int* displacements = new int[nApplications];
    int displacement = 0;
    for (int i = 0; i < nApplications; ++i)
      {
	int receiveSize = negotiationDataSize (nConnections[i]);
	receiveSizes[i] = receiveSize;
	displacements[i] = displacement;
	TemporalNegotiationData* data =
	  static_cast<TemporalNegotiationData*>
	  (static_cast<void*> (memory + displacement));
	nodes.push_back (data);
	displacement += receiveSize;
      }
    delete[] nConnections;
    int sendSize = negotiationDataSize (nLocalConnections);
    negotiationComm.Allgatherv (negotiationData, sendSize, MPI::BYTE,
				negotiationBuffer, receiveSizes, displacements,
				MPI::BYTE);
    delete[] displacements;
    delete[] receiveSizes;
    freeNegotiationData (negotiationData);
    negotiationData = nodes[localNode];
  }


  ConnectionDescriptor*
  TemporalNegotiator::findInputConnection (int node, int port)
  {
    int nOut = nodes[node]->nOutConnections;
    // Get address of first input ConnectionDescriptor
    ConnectionDescriptor* inputDescriptors = &nodes[node]->connection[nOut];
    for (int i = 0; i < nodes[node]->nInConnections; ++i)
      if (inputDescriptors[i].receiverPort == port)
	return &inputDescriptors[i];
    error ("internal error in TemporalNegotiator::findInputConnection");
  }

  
  void
  TemporalNegotiator::combineParameters ()
  {
    for (int o = 0; o < nApplications; ++o)
      for (int c = 0; c < nodes[o]->nOutConnections; ++c)
	{
	  ConnectionDescriptor* out = &nodes[o]->connection[c];
	  int i = out->remoteNode;
	  ConnectionDescriptor* in = findInputConnection (i, out->receiverPort);
	  
	  // maxBuffered

	  // check defaults
	  if (out->maxBuffered == MAX_BUFFERED_NO_VALUE
	      && in->maxBuffered == MAX_BUFFERED_NO_VALUE)
	    out->maxBuffered = out->defaultMaxBuffered;
	  else if (in->maxBuffered != MAX_BUFFERED_NO_VALUE)
	    {
	      // convert to sender side ticks
	      ClockStateT inMaxBufferedTime
		= in->maxBuffered * nodes[i]->tickInterval;
	      int inMaxBuffered = inMaxBufferedTime / nodes[o]->tickInterval;
	      // take min maxBuffered
	      if (inMaxBuffered < out->maxBuffered)
		out->maxBuffered = inMaxBuffered;
	    }
	  // store maxBuffered in sender units
	  in->maxBuffered = out->maxBuffered;
	  
	  // accLatency
	  out->accLatency = in->accLatency;
	  
	  // remoteTickInterval
	  out->remoteTickInterval = nodes[i]->tickInterval;
	  in->remoteTickInterval = nodes[o]->tickInterval;
	}
  }

  
  void
  TemporalNegotiator::loopAlgorithm ()
  {
  }

  
  void
  TemporalNegotiator::broadcastNegotiationData ()
  {
    MPI::Intracomm comm = setup_->communicator ();
    comm.Bcast (&nLocalConnections, 1, MPI::INT, 0);
    comm.Bcast (negotiationData,
		negotiationDataSize (nLocalConnections), MPI::BYTE, 0);
  }


  void
  TemporalNegotiator::receiveNegotiationData ()
  {
    MPI::Intracomm comm = setup_->communicator ();
    comm.Bcast (&nLocalConnections, 1, MPI::INT, 0);
    negotiationBuffer = allocNegotiationData (1, nLocalConnections);
    negotiationData = negotiationBuffer;
    comm.Bcast (negotiationData,
		negotiationDataSize (nLocalConnections), MPI::BYTE, 0);
  }


  void
  TemporalNegotiator::distributeNegotiationData (Clock& localTime)
  {
    int nOut = negotiationData->nOutConnections;
    for (int i = 0; i < nOut; ++i)
      {
	int maxBuffered = negotiationData->connection[i].maxBuffered;
	int accLatency = negotiationData->connection[i].accLatency;
	ClockStateT remoteTickInterval
	  = negotiationData->connection[i].remoteTickInterval;
	OutputSynchronizer* synch
	  = outputConnections[i].connector ()->synchronizer ();
	synch->setLocalTime (&localTime);
	synch->setReceiverTickInterval (remoteTickInterval);
	synch->setMaxBuffered (maxBuffered);
	synch->setAccLatency (accLatency);
      }
    int nIn = negotiationData->nInConnections;
    for (int i = 0; i < nIn; ++i)
      {
	int maxBuffered = negotiationData->connection[nOut + i].maxBuffered;
	int accLatency = negotiationData->connection[nOut + i].accLatency;
	ClockStateT remoteTickInterval
	  = negotiationData->connection[i].remoteTickInterval;
	InputSynchronizer* synch
	  = inputConnections[i].connector ()->synchronizer ();
	synch->setLocalTime (&localTime);
	synch->setSenderTickInterval (remoteTickInterval);
	synch->setMaxBuffered (maxBuffered);
	synch->setAccLatency (accLatency);
      }
  }


  void
  TemporalNegotiator::negotiate (Clock& localTime)
  {
    createNegotiationCommunicator ();
    if (isLeader ())
      {
	collectNegotiationData (localTime.tickInterval ());
	communicateNegotiationData ();
	combineParameters ();
	loopAlgorithm ();
	if (hasPeers ())
	  broadcastNegotiationData ();
      }
    else
      receiveNegotiationData ();
    distributeNegotiationData (localTime);
  }
  
}

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
    ClockState integerLatency = accLatency / setup_->timebase () + 0.5;
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
						 ClockState tickInterval,
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
  TemporalNegotiator::collectNegotiationData (ClockState ti)
  {
    int nOut = outputConnections.size ();
    int nIn = inputConnections.size ();
    nLocalConnections = nOut + nIn;
    MUSIC_LOG ("nLocalConnections = " << nLocalConnections
	       << ", nOut = " << nOut
	       << ", nIn = " << nIn);
    negotiationData = allocNegotiationData (1, nLocalConnections);
    negotiationData->timebase = setup_->timebase ();
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
	nodes.push_back (ApplicationNode (this, i, data));
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
    negotiationData = nodes[localNode].data;
  }


  ConnectionDescriptor*
  TemporalNegotiator::findInputConnection (int node, int port)
  {
    int nOut = nodes[node].data->nOutConnections;
    // Get address of first input ConnectionDescriptor
    ConnectionDescriptor* inputDescriptors
      = &nodes[node].data->connection[nOut];
    for (int i = 0; i < nodes[node].data->nInConnections; ++i)
      if (inputDescriptors[i].receiverPort == port)
	return &inputDescriptors[i];
    error ("internal error in TemporalNegotiator::findInputConnection");
  }

  
  void
  TemporalNegotiator::combineParameters ()
  {
    double timebase = nodes[0].data->timebase;
    for (int o = 0; o < nApplications; ++o)
      {
	// check timebase
	if (nodes[o].data->timebase != timebase)
	  error0 ("applications don't use same timebase");
	
	for (int c = 0; c < nodes[o].data->nOutConnections; ++c)
	  {
	    ConnectionDescriptor* out = &nodes[o].data->connection[c];
	    int i = out->remoteNode;
	    ConnectionDescriptor* in = findInputConnection (i,
							    out->receiverPort);
	  
	    // maxBuffered

	    // check defaults
	    if (out->maxBuffered == MAX_BUFFERED_NO_VALUE
		&& in->maxBuffered == MAX_BUFFERED_NO_VALUE)
	      out->maxBuffered = out->defaultMaxBuffered;
	    else if (in->maxBuffered != MAX_BUFFERED_NO_VALUE)
	      {
		// convert to sender side ticks
		ClockState inMaxBufferedTime
		  = in->maxBuffered * nodes[i].data->tickInterval;
		int inMaxBuffered = (inMaxBufferedTime
				     / nodes[o].data->tickInterval);
		// take min maxBuffered
		if (inMaxBuffered < out->maxBuffered)
		  out->maxBuffered = inMaxBuffered;
	      }
	    // store maxBuffered in sender units
	    in->maxBuffered = out->maxBuffered;
	  
	    // accLatency
	    out->accLatency = in->accLatency;
	  
	    // remoteTickInterval
	    out->remoteTickInterval = nodes[i].data->tickInterval;
	    in->remoteTickInterval = nodes[o].data->tickInterval;
	  }
      }
  }


  void
  TemporalNegotiator::depthFirst (ApplicationNode& x,
				  std::vector<Connection>& path)
  {
    if (x.inPath)
      {
	// Search path to detect beginning of loop
	int loop;
	for (loop = 0; &path[loop].pre () != &x; ++loop)
	  ;

	// Compute how much headroom we have for buffering
	ClockState totalDelay = 0;
	for (int c = loop; c < path.size (); ++c)
	  totalDelay += path[c].latency () - path[c].pre ().tickInterval ();

	// If negative we will not be able to make it in time
        // around the loop even without any extra buffering
        if (totalDelay < 0)
	  {
	    std::ostringstream ostr;
            ostr << "too short latency (" << - timebase * totalDelay
		 << " s) around loop: " << path[loop].pre ().name ();
	    for (int c = loop + 1; c < path.size (); ++c)
	      ostr << ", " << path[c].pre ().name ();
	    error0 (ostr.str ());
	  }

        // Distribute totalDelay as allowed buffering uniformly over loop
        // (we could do better by considering constraints form other loops)
	int loopLength = path.size () - loop;
        ClockState bufDelay = totalDelay / loopLength;
        for (int c = 0; c < path.size (); ++c)
	  {
	    int allowedTicks = bufDelay / path[c].pre ().tickInterval ();
	    path[c].setAllowedBuffer (std::min (path[c].allowedBuffer (),
						allowedTicks));
	  }

        return;
      }

    // Mark as processed (remove from main loop forest)
    x.visited = true;
    x.inPath = true;

    // Recurse in depth-first order
    for (int c = 0; c < x.nConnections (); ++c)
      {
	path.push_back (x.connection (c));
	depthFirst (x.connection (c).post (), path);
	path.pop_back ();
      }

    x.inPath = false;
  }
  
  
  void
  TemporalNegotiator::loopAlgorithm ()
  {
    std::vector<Connection> path;
    for (std::vector<ApplicationNode>::iterator node = nodes.begin ();
	 node != nodes.end ();
	 ++node)
      if (!node->visited)
	depthFirst (*node, path);
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
	ClockState remoteTickInterval
	  = negotiationData->connection[i].remoteTickInterval;
	Synchronizer* synch
	  = outputConnections[i].connector ()->synchronizer ();
	synch->setLocalTime (&localTime);
	// setReceiverTickInterval must be called *after* setLocalTime
	synch->setReceiverTickInterval (remoteTickInterval);
	synch->setMaxBuffered (maxBuffered);
	synch->setAccLatency (accLatency);
      }
    int nIn = negotiationData->nInConnections;
    for (int i = 0; i < nIn; ++i)
      {
	int maxBuffered = negotiationData->connection[nOut + i].maxBuffered;
	int accLatency = negotiationData->connection[nOut + i].accLatency;
	ClockState remoteTickInterval
	  = negotiationData->connection[i].remoteTickInterval;
	Synchronizer* synch
	  = inputConnections[i].connector ()->synchronizer ();
	synch->setLocalTime (&localTime);
	// setSenderTickInterval must be called *after* setLocalTime
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


  std::string
  ApplicationNode::name ()
  {
    // only used for error messages
    return (*negotiator_->setup ()->applicationMap ())[index].name ();
  };
  
}

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
#include "music/temporal.hh"

namespace MUSIC {

  TemporalNegotiator::TemporalNegotiator (Setup* setup)
    : setup_ (setup)
  {
    applicationRank = setup_->communicator ().Get_rank ();
  }


  void
  TemporalNegotiator::addConnection (OutputConnector* connector,
				     int maxBuffered)
  {
    outputConnections.push_back (OutputConnection (connector,
						   maxBuffered));
  }
  
  void
  TemporalNegotiator::addConnection (InputConnector* connector,
				     int maxBuffered,
				     int accLatency)
  {
    inputConnections.push_back (InputConnection (connector,
						 maxBuffered,
						 accLatency));
  }


  bool
  TemporalNegotiator::isLeader ()
  {
    return applicationRank == 0;
  }


  void
  TemporalNegotiator::collectNegotiationData (double timebase, ClockStateT ti)
  {
    
  }


  void
  TemporalNegotiator::communicateNegotiationData ()
  {
  }

  
  void
  TemporalNegotiator::combineParameters ()
  {
  }

  
  void
  TemporalNegotiator::loopAlgorithm ()
  {
  }

  
  void
  TemporalNegotiator::broadcastNegotiationData ()
  {
  }


  void
  TemporalNegotiator::receiveNegotiationData ()
  {
#if 0
    MPI::Intracomm comm = setup_->communicator ();
    int size;
    comm.Bcast (&size, 1, MPI::INTEGER, 0);
    void* buffer = static_cast<void*> (new char[size]);
    negotiationData = static_cast<TemporalNegotiationData*> (buffer);
    comm.Bcast (negotiationData, size, MPI::BYTE, 0);
#endif
  }


  void
  TemporalNegotiator::distributeNegotiationData (Clock& localTime)
  {
    int nOut = negotiationData->nOutConnections;
    for (int i = 0; i < nOut; ++i)
      {
#if 0
	int maxBuffered = negotiationData->connection[i].maxBuffered;
	int accLatency = negotiationData->connection[i].accLatency;
#endif
	OutputSynchronizer* synch
	  = outputConnections[i].connector ()->synchronizer ();
	synch->setLocalTime (&localTime);
#if 0
	synch->setMaxBuffered (maxBuffered);
	synch->setAccLatency (accLatency);
#endif
      }
    int nIn = negotiationData->nInConnections;
    for (int i = 0; i < nIn; ++i)
      {
#if 0
	int maxBuffered = negotiationData->connection[nOut + i].maxBuffered;
	int accLatency = negotiationData->connection[nOut + i].accLatency;
#endif
	InputSynchronizer* synch
	  = inputConnections[i].connector ()->synchronizer ();
	synch->setLocalTime (&localTime);
#if 0
	synch->setMaxBuffered (maxBuffered);
	synch->setAccLatency (accLatency);
#endif
      }
  }


  void
  TemporalNegotiator::negotiate (Clock& localTime)
  {
    if (isLeader ())
      {
	collectNegotiationData (localTime.timebase (),
				localTime.tickInterval ());
	communicateNegotiationData ();
	combineParameters ();
	loopAlgorithm ();
	broadcastNegotiationData ();
      }
    else
      receiveNegotiationData ();
    distributeNegotiationData (localTime);
  }
  
}

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

#ifndef MUSIC_TEMPORAL_HH

#define DEFAULT_PACKET_SIZE 64000

#include <music/clock.hh>

namespace MUSIC {

  class Setup;

  class OutputConnection {
  private:
    OutputConnector* connector_;
    int maxBuffered_;
  public:
    OutputConnection (OutputConnector* connector, int maxBuffered)
      : connector_ (connector), maxBuffered_ (maxBuffered) { }
    OutputConnector* connector () { return connector_; }
    int maxBuffered () { return maxBuffered_; }
  };
  
  class InputConnection {
  private:
    InputConnector* connector_;
    int maxBuffered_;
    ClockStateT accLatency_;
  public:
    InputConnection (InputConnector* connector,
		     int maxBuffered,
		     ClockStateT accLatency)
      : connector_ (connector),
	maxBuffered_ (maxBuffered),
	accLatency_ (accLatency) { }
    InputConnector* connector () { return connector_; }
    int maxBuffered () { return maxBuffered_; }
    ClockStateT accLatency () { return accLatency_; }
  };
  
  class ConnectionDescriptor {
  public:
    int remoteNode;
    int receiverPort;
    int maxBuffered;
    ClockStateT accLatency;
    ClockStateT remoteTickInterval;
  };
  
  class TemporalNegotiationData {
  public:
    double timeBase;
    ClockStateT tickInterval;
    int nOutConnections;
    int nInConnections;
    int recNamesSize;
    ConnectionDescriptor connection[0];
  };

  class TemporalNegotiator {
    Setup* setup_;
    MPI::Intracomm negotiationComm;
    std::map<int, int> leaderToNode;
    int nApplications; // initialized by createNegotiationCommunicator
    int nLocalConnections;
    int localNode;
    std::vector<OutputConnection> outputConnections;
    std::vector<InputConnection> inputConnections;
    std::vector<TemporalNegotiationData*> nodes;
    TemporalNegotiationData* negotiationBuffer;
    TemporalNegotiationData* negotiationData;
    int negotiationDataSize (int nConnections);
    int negotiationDataSize (int nBlock, int nConnections);
    TemporalNegotiationData* allocNegotiationData (int nBlocks,
						   int nConnections);
    void freeNegotiationData (TemporalNegotiationData*);
    ConnectionDescriptor* findInputConnection (int node, int port);
    bool isLeader ();
    bool hasPeers ();
  public:
    TemporalNegotiator (Setup* setup);
    ~TemporalNegotiator ();
    void addConnection (OutputConnector* connector, int maxBuffered);
    void addConnection (InputConnector* connector,
			int maxBuffered,
			double accLatency);
    void createNegotiationCommunicator ();
    void collectNegotiationData (ClockStateT ti);
    void communicateNegotiationData ();
    void combineParameters ();
    void loopAlgorithm ();
    void broadcastNegotiationData ();
    void receiveNegotiationData ();
    void distributeNegotiationData (Clock& localTime);
    void negotiate (Clock& localTime);
  };

}

#define MUSIC_TEMPORAL_HH
#endif

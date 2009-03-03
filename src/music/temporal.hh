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

#define MAX_BUFFERED_NO_VALUE 0
#define DEFAULT_PACKET_SIZE 64000
#define EVENT_FREQUENCY_ESTIMATE 10.0

#include <music/clock.hh>

namespace MUSIC {

  class Setup;

  class OutputConnection {
  private:
    Connector* connector_;
    int maxBuffered_;
    int elementSize_;
  public:
    OutputConnection (Connector* connector,
		      int maxBuffered,
		      int elementSize)
      : connector_ (connector),
	maxBuffered_ (maxBuffered),
	elementSize_ (elementSize) { }
    Connector*& connector () { return connector_; }
    int maxBuffered () { return maxBuffered_; }
    int elementSize () { return elementSize_; }
  };
  
  class InputConnection {
  private:
    Connector* connector_;
    int maxBuffered_;
    ClockState accLatency_;
    bool interpolate_;
  public:
    InputConnection (Connector* connector,
		     int maxBuffered,
		     ClockState accLatency,
		     bool interpolate)
      : connector_ (connector),
	maxBuffered_ (maxBuffered),
	accLatency_ (accLatency),
	interpolate_ (interpolate) { }
    Connector*& connector () { return connector_; }
    int maxBuffered () { return maxBuffered_; }
    ClockState accLatency () { return accLatency_; }
    bool interpolate () { return interpolate_; }
  };
  
  class ConnectionDescriptor {
  public:
    int remoteNode;
    int receiverPort;
    int maxBuffered;
    int defaultMaxBuffered; // not used for input connections
    bool interpolate;
    ClockState accLatency;
    ClockState remoteTickInterval;
  };
  
  class TemporalNegotiationData {
  public:
    double timebase;
    ClockState tickInterval;
    ClockState maxDelay;
    int nOutConnections;
    int nInConnections;
    ConnectionDescriptor connection[0];
  };

  class ApplicationNode;

  class Connection;

  class TemporalNegotiator {
    Setup* setup_;
    MPI::Intracomm negotiationComm;
    std::map<int, int> leaderToNode;
    int nApplications; // initialized by createNegotiationCommunicator
    int nLocalConnections;
    int localNode;
    double timebase;
    std::vector<OutputConnection> outputConnections;
    std::vector<InputConnection> inputConnections;
    std::vector<ApplicationNode> nodes;
    TemporalNegotiationData* negotiationBuffer;
    TemporalNegotiationData* negotiationData;
    int negotiationDataSize (int nConnections);
    int negotiationDataSize (int nBlock, int nConnections);
    int computeDefaultMaxBuffered (int maxLocalWidth,
				   int eventSize,
				   ClockState tickInterval,
				   double timebase);
    TemporalNegotiationData* allocNegotiationData (int nBlocks,
						   int nConnections);
    void freeNegotiationData (TemporalNegotiationData*);
    ConnectionDescriptor* findInputConnection (int node, int port);
    bool isLeader ();
    bool hasPeers ();
    void depthFirst (ApplicationNode& x,
		     std::vector<Connection>& path);
  public:
    TemporalNegotiator (Setup* setup);
    ~TemporalNegotiator ();
    Setup* setup () { return setup_; }
    ApplicationNode& applicationNode (int i) { return nodes[i]; }
    void addConnection (OutputConnector* connector,
			int maxBuffered,
			int elementSize);
    void addConnection (InputConnector* connector,
			int maxBuffered,
			double accLatency,
			bool interpolate);
    void createNegotiationCommunicator ();
    void collectNegotiationData (ClockState ti);
    void communicateNegotiationData ();
    void combineParameters ();
    void loopAlgorithm ();
    void broadcastNegotiationData ();
    void receiveNegotiationData ();
    void distributeNegotiationData (Clock& localTime);
    void negotiate (Clock& localTime);
  };

  class Connection {
    ApplicationNode* pre_;
    ApplicationNode* post_;
    ConnectionDescriptor* connection_;
  public:
    Connection (ApplicationNode& pre,
		ApplicationNode& post,
		ConnectionDescriptor& descr)
      : pre_ (&pre), post_ (&post), connection_ (&descr) { }
    ApplicationNode& pre () { return *pre_; }
    ApplicationNode& post () { return *post_; }
    ClockState latency () { return connection_->accLatency;}
    int allowedBuffer () { return connection_->maxBuffered; }
    void setAllowedBuffer (int a) { connection_->maxBuffered = a; }
  };

  class ApplicationNode {
    TemporalNegotiator* negotiator_;
    int index;
  public:
    ApplicationNode (TemporalNegotiator* negotiator,
		     int i,
		     TemporalNegotiationData* data_)
      : negotiator_ (negotiator), index (i), data (data_)
    {
      visited = false;
      inPath = false;
    }
    TemporalNegotiationData* data;
    bool visited;
    bool inPath;
    std::string name ();
    ClockState tickInterval () { return data->tickInterval; }
    int nConnections () { return data->nOutConnections; }
    Connection connection (int c)
    {
      ConnectionDescriptor& descr = data->connection[c];
      return Connection (*this,
			 negotiator_->applicationNode (descr.remoteNode),
			 descr);
    };
  };

}

#define MUSIC_TEMPORAL_HH
#endif

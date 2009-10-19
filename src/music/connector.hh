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

#ifndef MUSIC_CONNECTOR_HH

#include <mpi.h>

#include <vector>
#include <string>

#include <music/synchronizer.hh>
#include <music/FIBO.hh>
#include <music/event.hh>
#include <music/spatial.hh>
#include <music/connectivity.hh>
#include <music/sampler.hh>
#include <music/collector.hh>
#include <music/distributor.hh>
#include <music/event_router.hh>

#include <music/subconnector.hh>

namespace MUSIC {

  // The Connector is responsible for one side of the communication
  // between the ports of a port pair.  An output port can have
  // multiple connectors while an input port only has one.  The method
  // connector::connect () creates one subconnector for each MPI
  // process we will communicate with on the remote side.

  class Connector {
  protected:
    ConnectorInfo info;
    SpatialNegotiator* spatialNegotiator_;
    MPI::Intracomm comm;
    MPI::Intercomm intercomm;
    
  public:
    Connector () { }
    Connector (ConnectorInfo info_,
	       SpatialNegotiator* spatialNegotiator_,
	       MPI::Intracomm c);
    Connector (ConnectorInfo info_,
	       SpatialNegotiator* spatialNegotiator_,
	       MPI::Intracomm c,
	       MPI::Intercomm ic);
    virtual ~Connector () { }
    virtual Connector* specialize (Clock& localTime) { return this; }

    std::string receiverAppName () const { return info.receiverAppName (); }
    std::string receiverPortName () const { return info.receiverPortName (); }
    int receiverPortCode () const { return info.receiverPortCode (); }
    int remoteLeader () const { return info.remoteLeader (); }
    
    int maxLocalWidth () { return spatialNegotiator_->maxLocalWidth (); }
    bool isLeader ();
    virtual Synchronizer* synchronizer () = 0;
    void createIntercomm ();
    void freeIntercomm ();
    virtual void
    spatialNegotiation (std::vector<OutputSubconnector*>& osubconn,
			std::vector<InputSubconnector*>& isubconn) { }
    virtual void initialize () = 0;
    virtual void prepareForSimulation () { }
    virtual void tick (bool& requestCommunication) = 0;
  };

  class PostCommunicationConnector : virtual public Connector {
  public:
    virtual void postCommunication () = 0;    
  };

  class OutputConnector : virtual public Connector {
  public:
    virtual void spatialNegotiation (std::vector<OutputSubconnector*>& osubconn,
				     std::vector<InputSubconnector*>& isubconn);
    virtual void addRoutingInterval (IndexInterval i, OutputSubconnector* s);
    virtual OutputSubconnector* makeOutputSubconnector (int remoteRank) = 0;
  };
  
  class InputConnector : virtual public Connector {
  public:
    virtual void spatialNegotiation (std::vector<OutputSubconnector*>& osubconn,
				     std::vector<InputSubconnector*>& isubconn);
    virtual void addRoutingInterval (IndexInterval i, InputSubconnector* s);
    virtual InputSubconnector* makeInputSubconnector (int remoteRank,
						      int receiverRank) = 0;
  };

  class ContConnector : virtual public Connector {
  protected:
    Sampler& sampler_;
    MPI::Datatype type_;
    // We need to allocate instances of ContOutputConnector and
    // ContInputConnector and, therefore need dummy versions of the
    // following virtual functions:
    virtual Synchronizer* synchronizer () { return NULL; };
    virtual void initialize () { }
    virtual void tick (bool&) { }
  public:
    ContConnector (Sampler& sampler, MPI::Datatype type)
      : sampler_ (sampler), type_ (type) { }
    ClockState remoteTickInterval (ClockState tickInterval);
  };  
  
  class InterpolatingConnector : virtual public Connector {
  };
  
  class ContOutputConnector : public ContConnector, public OutputConnector {
  protected:
    Distributor distributor_;
  public:
    ContOutputConnector (ConnectorInfo connInfo,
			 SpatialNegotiator* spatialNegotiator,
			 MPI::Intracomm comm,
			 Sampler& sampler,
			 MPI::Datatype type);
    OutputSubconnector* makeOutputSubconnector (int remoteRank);
    void addRoutingInterval (IndexInterval i, OutputSubconnector* osubconn);
    Connector* specialize (Clock& localTime);
  };
  
  class PlainContOutputConnector : public ContOutputConnector {
    OutputSynchronizer synch;
  public:
    PlainContOutputConnector (ContOutputConnector& connector);
    Synchronizer* synchronizer () { return &synch; }
    void initialize ();
    void tick (bool& requestCommunication);
  };
  
  class InterpolatingContOutputConnector : public ContOutputConnector,
					   public InterpolatingConnector {
    InterpolationOutputSynchronizer synch;
  public:
    InterpolatingContOutputConnector (ContOutputConnector& connector);
    Synchronizer* synchronizer () { return &synch; }
    void initialize ();
    void tick (bool& requestCommunication);
  };
  
  class ContInputConnector : public ContConnector,
			     public InputConnector,
			     public PostCommunicationConnector {
  protected:
    Collector collector_;
    double delay_;
    bool divisibleDelay (Clock& localTime);
  public:
    ContInputConnector (ConnectorInfo connInfo,
			SpatialNegotiator* spatialNegotiator,
			MPI::Intracomm comm,
			Sampler& sampler,
			MPI::Datatype type,
			double delay);
    InputSubconnector* makeInputSubconnector (int remoteRank, int receiverRank);
    void addRoutingInterval (IndexInterval i, InputSubconnector* isubconn);
    Connector* specialize (Clock& localTime);
    // We need to allocate instances of ContInputConnector and, therefore
    // need dummy versions of the following virtual functions:
    virtual void postCommunication () { }
  };
  
  class PlainContInputConnector : public ContInputConnector {
    InputSynchronizer synch;
  public:
    PlainContInputConnector (ContInputConnector& connector);
    Synchronizer* synchronizer () { return &synch; }
    void initialize ();
    void tick (bool& requestCommunication);
    void postCommunication ();
  };

  class InterpolatingContInputConnector : public ContInputConnector,
					  public InterpolatingConnector {
    InterpolationInputSynchronizer synch;
    bool first_;
  public:
    InterpolatingContInputConnector (ContInputConnector& connector);
    Synchronizer* synchronizer () { return &synch; }
    void initialize ();
    void tick (bool& requestCommunication);
    void postCommunication ();
  };

  class EventConnector : virtual public Connector {
  };
  
  class EventOutputConnector : public OutputConnector, public EventConnector {
  private:
    OutputSynchronizer synch;
    EventRoutingMap* routingMap_;
    void send ();
  public:
    EventOutputConnector (ConnectorInfo connInfo,
			  SpatialOutputNegotiator* spatialNegotiator,
			  MPI::Intracomm comm,
			  EventRoutingMap* routingMap);
    OutputSubconnector* makeOutputSubconnector (int remoteRank);
    void addRoutingInterval (IndexInterval i, OutputSubconnector* osubconn);
    Synchronizer* synchronizer () { return &synch; }
    void initialize ();
    void tick (bool& requestCommunication);
  };
  
  class EventInputConnector : public InputConnector, public EventConnector {
    
  private:
    InputSynchronizer synch;
    EventHandlerPtr handleEvent_;
    Index::Type type_;
  public:
    EventInputConnector (ConnectorInfo connInfo,
			 SpatialInputNegotiator* spatialNegotiator,
			 EventHandlerPtr handleEvent,
			 Index::Type type,
			 MPI::Intracomm comm);
    InputSubconnector* makeInputSubconnector (int remoteRank, int receiverRank);
    Synchronizer* synchronizer () { return &synch; }
    void initialize ();
    void tick (bool& requestCommunication);
  };
  
  class MessageConnector : virtual public Connector {
  };
  
  class MessageOutputConnector : public OutputConnector,
				 public MessageConnector,
				 public PostCommunicationConnector {
  private:
    OutputSynchronizer synch;
    FIBO buffer;
    bool bufferAdded;
    std::vector<FIBO*>& buffers_;
    void send ();
  public:
    MessageOutputConnector (ConnectorInfo connInfo,
			    SpatialOutputNegotiator* spatialNegotiator,
			    MPI::Intracomm comm,
			    std::vector<FIBO*>& buffers);
    OutputSubconnector* makeOutputSubconnector (int remoteRank);
    void addRoutingInterval (IndexInterval i, OutputSubconnector* osubconn);
    Synchronizer* synchronizer () { return &synch; }
    void initialize ();
    void tick (bool& requestCommunication);
    void postCommunication ();
  };
  
  class MessageInputConnector : public InputConnector, public MessageConnector {
    
  private:
    InputSynchronizer synch;
    MessageHandler* handleMessage_;
    Index::Type type_;
  public:
    MessageInputConnector (ConnectorInfo connInfo,
			   SpatialInputNegotiator* spatialNegotiator,
			   MessageHandler* handleMessage,
			   Index::Type type,
			   MPI::Intracomm comm);
    InputSubconnector* makeInputSubconnector (int remoteRank, int receiverRank);
    Synchronizer* synchronizer () { return &synch; }
    void initialize ();
    void tick (bool& requestCommunication);
  };
  
}

#define MUSIC_CONNECTOR_HH
#endif

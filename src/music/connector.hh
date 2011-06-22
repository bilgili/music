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
#include <music/event_routingmap.hh>

#include <music/subconnector.hh>
namespace MUSIC {

  // The Connector is responsible for one side of the communication
  // between the ports of a port pair.  An output port can have
  // multiple connectors while an input port only has one.  The method
  // connector::connect () creates one subconnector for each MPI
  // process we will communicate with on the remote side.

  class Connector {
  public:
	  enum SubconnectorsType{OUTPUT_SUBCONNECTORS,INPUT_SUBCONNECTORS,COLLECTIVE_SUBCONNECTORS,UNDEFINED_SUBCONNECTORS};
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
    virtual void createIntercomm ();
    virtual void freeIntercomm ();
    virtual int spatialNegotiation (std::vector<Subconnector*>& subconn);
    virtual void initialize () = 0;
    virtual void prepareForSimulation () { }
    virtual void tick (bool& requestCommunication) = 0;

    virtual void addRoutingInterval (IndexInterval i, Subconnector* s){};
    virtual Subconnector* makeSubconnector (int remoteRank) = 0;
  };


  class PostCommunicationConnector : virtual public Connector {
  public:
    virtual void postCommunication () = 0;    
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
  
  class ContOutputConnector : public ContConnector {
  protected:
    Distributor distributor_;
  public:
    ContOutputConnector (ConnectorInfo connInfo,
			 SpatialNegotiator* spatialNegotiator,
			 MPI::Intracomm comm,
			 Sampler& sampler,
			 MPI::Datatype type);
    Subconnector* makeSubconnector (int remoteRank);
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
    Subconnector* makeSubconnector (int remoteRank);
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

  class EventConnector :  public Connector {
  protected:
	  Synchronizer *synch;
	  Index::Type type_;
  public:
	  EventConnector(ConnectorInfo connInfo,
			  SpatialNegotiator* spatialNegotiator,
			  MPI::Intracomm comm,
			  Index::Type type):Connector(connInfo,spatialNegotiator,comm),type_(type){}
	  virtual ~EventConnector(){};
	//  SpatialNegotiator *spatialNegotiator(){return spatialNegotiator_;}
	//  MPI::Intracomm communicator(){return comm;}
	  Synchronizer* synchronizer () { return synch; }
	  void initialize ();
	  void tick (bool& requestCommunication);
  protected:
	  virtual void createSynchronizer()=0;
	  virtual void destroySynchronizer()=0;
  };

  class EventOutputConnector : public EventConnector {
  private:
    EventRoutingMap<FIBO*>* routingMap_;
  public:
    EventOutputConnector (ConnectorInfo connInfo,
			  SpatialNegotiator* spatialNegotiator,
			  MPI::Intracomm comm,
			  Index::Type type,
			  EventRoutingMap<FIBO*>* routingMap):EventConnector(connInfo,spatialNegotiator,comm,type),
			  routingMap_(routingMap){createSynchronizer();};
    ~EventOutputConnector(){destroySynchronizer();}
    Subconnector* makeSubconnector (int remoteRank);
    void addRoutingInterval (IndexInterval i, Subconnector* subconn);
    int spatialNegotiation(std::vector<Subconnector*>& subconn);
  protected:
    void createSynchronizer();
    void destroySynchronizer();
  };
  
  class EventInputConnector : public EventConnector {
  private:
	EventRoutingMap<EventHandlerGlobalIndex*>* routingMap_;
    EventHandlerPtr handleEvent_;
  public:
    EventInputConnector (ConnectorInfo connInfo,
			  SpatialNegotiator* spatialNegotiator,
			  MPI::Intracomm comm,
			  Index::Type type,
			  EventRoutingMap<EventHandlerGlobalIndex*>* routingMap,
			  EventHandlerPtr handleEvent):EventConnector(connInfo,spatialNegotiator,comm,type),
			  routingMap_(routingMap),handleEvent_(handleEvent){createSynchronizer();};
    ~EventInputConnector(){destroySynchronizer();}
    Subconnector* makeSubconnector (int remoteRank);
    int spatialNegotiation(std::vector<Subconnector*>& subconn);
    void addRoutingInterval (IndexInterval i, Subconnector* subconn);

  protected:
    void createSynchronizer();
    void destroySynchronizer();
  };

 class CollectiveConnector: public Connector {
  	EventConnector *conn;
  	Subconnector *subconnector;
  	EventRouter *router_;
  	EventRouter *empty_router;
  public:
  	CollectiveConnector(ConnectorInfo connInfo,
  	 			  SpatialNegotiator* spatialNegotiator,
  	 			  MPI::Intracomm comm,
  	 			  EventRoutingMap<EventHandlerGlobalIndex *>* routingMap,
  	 			  EventHandlerPtr handleEvent,
  	 			  EventRouter *router):
  	 			  Connector(connInfo,spatialNegotiator,comm),
  	 			  subconnector(0),router_(router),empty_router(NULL)
  	{ conn = new EventInputConnector(connInfo,spatialNegotiator,comm, Index::UNDEFINED,routingMap,handleEvent);};
  	CollectiveConnector(ConnectorInfo connInfo,
  	  	 			  SpatialNegotiator* spatialNegotiator,
  	  	 			  MPI::Intracomm comm,
  	  	 			  EventRoutingMap<FIBO *>* routingMap):
  	  	 		      Connector(connInfo,spatialNegotiator,comm),
  	  	 			  subconnector(0)
  	{ conn = new EventOutputConnector(connInfo,spatialNegotiator,comm, Index::UNDEFINED,routingMap);
  	empty_router = new EventRouter();
  	router_ = empty_router;};
  	~CollectiveConnector(){delete conn;if(empty_router != NULL) delete empty_router;}
  	int spatialNegotiation (std::vector<Subconnector*>& subconn);
  	Synchronizer* synchronizer () { return conn->synchronizer(); }
  	void initialize (){conn->initialize();}
  	void tick(bool& requestCommunication){conn->tick(requestCommunication);}
  	void createIntercomm(){}
  	void freeIntercomm (){}
  private:
  	Subconnector* makeSubconnector (int remoteRank);
  	void addRoutingInterval(IndexInterval i, Subconnector* subconn);
    };

  class MessageOutputConnector : public PostCommunicationConnector {
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
    Subconnector* makeSubconnector (int remoteRank);
    void addRoutingInterval (IndexInterval i, OutputSubconnector* osubconn);
    Synchronizer* synchronizer () { return &synch; }
    void initialize ();
    void tick (bool& requestCommunication);
    void postCommunication ();
  };
  
  class MessageInputConnector : public Connector {
    
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
    Subconnector* makeSubconnector (int remoteRank);
    Synchronizer* synchronizer () { return &synch; }
    void initialize ();
    void tick (bool& requestCommunication);
  };
  
}

#define MUSIC_CONNECTOR_HH
#endif

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
#include "music/debug.hh"
#ifdef USE_MPI
#include <mpi.h>

#include <vector>
#include <string>

//#include <music/synchronizer.hh>
#include <music/FIBO.hh>
#include <music/event.hh>
#include <music/spatial.hh>
#include <music/connectivity.hh>
#include <music/sampler.hh>
#include <music/collector.hh>
#include <music/distributor.hh>
#include <music/event_routingmap.hh>
#include <music/clock.hh>
#include <music/subconnector.hh>
namespace MUSIC {
/* remedius
 * New type of Connector was introduced: CollectiveConnector.
 * So that Connector class hierarchy undergoes the following changes:
 * the common functionality for OutputConnector and InputConnector classes was moved to the base Connector class.
 */
  // The Connector is responsible for one side of the communication
  // between the ports of a port pair.  An output port can have
  // multiple connectors while an input port only has one.  The method
  // connector::connect () creates one subconnector for each MPI
  // process we will communicate with on the remote side.

   /* remedius
  */

  class Connector {

  protected:
    ConnectorInfo info;
    SpatialNegotiator* spatialNegotiator_;
    MPI::Intracomm comm;
    MPI::Intercomm intercomm;
    std::vector<Subconnector*> rsubconn;
    ClockState latency_;
    // interpolate rather than picking value closest in time
    bool interpolate_;

  public:
    Connector () { };
    Connector (ConnectorInfo info_,
	       SpatialNegotiator* spatialNegotiator_,
	       MPI::Intracomm c);
    Connector (ConnectorInfo info_,
	       SpatialNegotiator* spatialNegotiator_,
	       MPI::Intracomm c,
	       MPI::Intercomm ic);
    virtual ~Connector () {for (std::vector<Subconnector*>::iterator subconnector = rsubconn.begin ();
    		 subconnector != rsubconn.end ();
    		 ++subconnector)
    	      delete *subconnector; }
    virtual Connector* specialize (Clock& localTime) { return this; }

    std::string receiverAppName () const { return info.receiverAppName (); }
    std::string receiverPortName () const { return info.receiverPortName (); }
    int receiverPortCode () const { return info.receiverPortCode (); }
    int remoteLeader () const { return info.remoteLeader (); }
    
    int maxLocalWidth () { return spatialNegotiator_->maxLocalWidth (); }
    bool isLeader ();
    void setInterpolate(bool val){interpolate_ =  val;}
    void setLatency( ClockState latency){ latency_ = latency;}
   // virtual Synchronizer* synchronizer () = 0;
    virtual void createIntercomm ();
    virtual void freeIntercomm ();

    /* remedius
     * since there can be different types of Subconnectors,
     * Instead of adding new type of Subconnector,
     * function parameters' were changed to one parameter without specifying the type of Subconnector and
     * the common functionality for InputConnector::spatialNegotiation and OutputConnector::spatialNegotiation
     * was coded in this base Connector::spatialNegotiation() method.
     * Returns SubconnectorType.
     */
    virtual void spatialNegotiation ();
    /* remedius
     *  sorts its vector of subconnectors according to its world rank.
     */
    virtual void initialize ();

    /* remedius
     * finalizeSimulation method iterates its subconnectors and calls accordring flush method
     */
    bool finalizeSimulation();
    /* remedius
     * tick method iterates its subconnectors and call according function to perform communication;
     * requestCommunication method argument was removed since there is no need anymore to check for next communication.
     * Communication is scheduled by separate Scheduler objects that Runtime object contains.
     */
    virtual void tick ();
    /* remedius
     * In order to carry out the base functionality to the spatialNegotiation() method
     * the two following common virtual methods were added so that
     * all successors of this class had to override abstract makeSubconnector method
     * instead of implementing it's own make^SpecificType^Subconnector() method.
     */
    virtual void addRoutingInterval (IndexInterval i, Subconnector* s){};
    virtual Subconnector* makeSubconnector (int remoteRank) = 0;
  };


  class PostCommunicationConnector : virtual public Connector {
  public:
    virtual void postCommunication () = 0;    
  };
  class PreCommunicationConnector : virtual public Connector {
    public:
      virtual void preCommunication () = 0;
    };
  class OutputConnector : virtual public Connector {
  public:
  };

  class InputConnector : virtual public Connector {
  public:
  };

  class ContConnector : virtual public Connector {
  protected:
    Sampler& sampler_;
    MPI::Datatype type_;

    Clock *localTime_;
    Clock remoteTime_;

    // We need to allocate instances of ContOutputConnector and
    // ContInputConnector and, therefore need dummy versions of the
    // following virtual functions:
  //  virtual Synchronizer* synchronizer () { return NULL; };
   // virtual void initialize () { }
    //virtual void tick () { }
  public:
    ContConnector (Sampler& sampler, MPI::Datatype type)
      : sampler_ (sampler), type_ (type),localTime_(NULL)  { }
    ClockState remoteTickInterval (ClockState tickInterval);
    virtual void initialize(){Connector::initialize(); initialCommunication();};
  protected:
    virtual void initialCommunication()=0;
    void remoteTick();
  };  
  
  class InterpolatingConnector : virtual public Connector {
  };
  
  class ContOutputConnector : public ContConnector, public OutputConnector, public PreCommunicationConnector  {
  protected:
    Distributor distributor_;
  public:
    ContOutputConnector (ConnectorInfo connInfo,
			 SpatialNegotiator* spatialNegotiator,
			 MPI::Intracomm comm,
			 Sampler& sampler,
			 MPI::Datatype type);
    Subconnector* makeSubconnector (int remoteRank);
    void addRoutingInterval (IndexInterval i, Subconnector* osubconn);
    Connector* specialize (Clock& localTime);
  protected:
    void initialCommunication();
    virtual void preCommunication(){};

  };
  
  class PlainContOutputConnector : public ContOutputConnector {
   // OutputSynchronizer synch;
  public:
    PlainContOutputConnector (ContOutputConnector& connector);
  //  Synchronizer* synchronizer () { return &synch; }
    void initialize ();
    void preCommunication();
  private:
    bool sample();

  };
  
  class InterpolatingContOutputConnector : public ContOutputConnector,
					   public InterpolatingConnector {
   // InterpolationOutputSynchronizer synch;
  public:
    InterpolatingContOutputConnector (ContOutputConnector& connector);
   // Synchronizer* synchronizer () { return &synch; }
    void initialize ();
    void preCommunication();
  private:

    bool interpolate();
    bool sample();
    double interpolationCoefficient();

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
    Subconnector* makeSubconnector (int remoteRank);
    void addRoutingInterval (IndexInterval i, Subconnector* isubconn);
    Connector* specialize (Clock& localTime);
    // We need to allocate instances of ContInputConnector and, therefore
    // need dummy versions of the following virtual functions:
    virtual void postCommunication () { }
  protected:
	// receive first chunk of data from sender application and fill
	// cont buffers according to Synchronizer::initialBufferedTicks ()
    void initialCommunication();
  private:
    int initialBufferedTicks();
  };
  
  class PlainContInputConnector : public ContInputConnector {
 //   InputSynchronizer synch;
  public:
    PlainContInputConnector (ContInputConnector& connector);
 //   Synchronizer* synchronizer () { return &synch; }
    void initialize ();
    void postCommunication ();

  };

  class InterpolatingContInputConnector : public ContInputConnector,
					  public InterpolatingConnector {
   // InterpolationInputSynchronizer synch;
    bool first_;
  public:
    InterpolatingContInputConnector (ContInputConnector& connector);
   // Synchronizer* synchronizer () { return &synch; }
    void initialize ();
    void postCommunication ();
  private:
    bool sample();
    double interpolationCoefficient();
  };

  class EventConnector : virtual public Connector {
    };

  class EventOutputConnector : public OutputConnector, public EventConnector {
  private:
	//OutputSynchronizer synch;
    EventRoutingMap<FIBO*>* routingMap_;
  public:
    EventOutputConnector (ConnectorInfo connInfo,
			  SpatialNegotiator* spatialNegotiator,
			  MPI::Intracomm comm,
			  EventRoutingMap<FIBO*>* routingMap): Connector(connInfo,spatialNegotiator,comm),
			  routingMap_(routingMap){};
    ~EventOutputConnector(){}
    Subconnector* makeSubconnector (int remoteRank);
    void addRoutingInterval (IndexInterval i, Subconnector* subconn);
  };
  /* remedius
   * routingMap field was added to the EventInputConnector class
   * in order to make it usable both for collective and pairwise algorithms'.
   */
  class EventInputConnector : public InputConnector, public EventConnector {
  private:
	//InputSynchronizer synch;
//	EventRoutingMap<EventHandlerGlobalIndex*>* routingMap_;
    EventHandlerPtr handleEvent_;
    Index::Type type_;
  public:

    EventInputConnector (ConnectorInfo connInfo,
			  SpatialNegotiator* spatialNegotiator,
			  MPI::Intracomm comm,
			  Index::Type type,
			  EventHandlerPtr handleEvent):Connector(connInfo,spatialNegotiator,comm),
			  handleEvent_(handleEvent),type_(type){};
    ~EventInputConnector(){}
    Subconnector* makeSubconnector (int remoteRank);
    /* remedius
     * original version of the following function does nothing,
     * however this function could perform inserting interval to the routingMap_ as well,
     * that is currently done in CollectiveConnector::addRoutingInterval method in order to keep original functionality.
     * Otherwise this could give an opportunity to have different event handlers for different ranges of indexes as in collective algorithm.
     * Do we need this? If not, then it's better to leave as it is.
     */
    void addRoutingInterval (IndexInterval i, Subconnector* subconn);
  };
/* remedius
 * New class CollectiveConnector was introduced in order to create CollectiveSubconnector object.
 * The difference between EventInputConnector, EventOutputConnector instances and  CollectiveConnector is
 * the latest creates one CollectiveSubconnector responsible for the collective communication
 * among all ranks on the output and input side, while EventInputConnector and EventOutputConnector
 * instances are responsible for creating as much Subconnectors as necessary for each point2point connectivity
 * for output and input sides.
 */
 class CollectiveConnector:  public EventConnector {
  	//Connector *conn;
  	Subconnector *subconnector;
  	EventRoutingMap<EventHandlerGlobalIndex*>* routingMap_input;
  	EventRoutingMap<FIBO *>* routingMap_output;
  	EventHandlerPtr handleEvent_;
  	EventRouter *router_;
  	EventRouter *empty_router;
  	bool high;
  	Index::Type type_;
  public:
  	CollectiveConnector(ConnectorInfo connInfo,
  	 			  SpatialNegotiator* spatialNegotiator,
  	 			  MPI::Intracomm comm,
  	 			  EventRoutingMap<EventHandlerGlobalIndex *>* routingMap,
  	 			  EventHandlerPtr handleEvent,
  	 			  EventRouter *router,
  	 			  Index::Type type):
  	 			  Connector(connInfo,spatialNegotiator,comm),
  	 			  subconnector(0),  routingMap_input(routingMap), routingMap_output(NULL),handleEvent_(handleEvent),router_(router),empty_router(NULL),high(true),type_(type)
  	{ };//conn = new EventInputConnector(connInfo,spatialNegotiator,comm, Index::UNDEFINED,handleEvent);};
  	CollectiveConnector(ConnectorInfo connInfo,
  	  	 			  SpatialNegotiator* spatialNegotiator,
  	  	 			  MPI::Intracomm comm,
  	  	 			  EventRoutingMap<FIBO *>* routingMap):
  	  	 		      Connector(connInfo, spatialNegotiator,comm),
  	  	 			  subconnector(0),routingMap_input(NULL), routingMap_output(routingMap), high(false),type_( Index::UNDEFINED)
  	{ //conn = new EventOutputConnector(connInfo,spatialNegotiator,comm,routingMap);
  	// we decided to perform event processing on the receiver side in the collective communication algorithm,
  	// so that there is no need for the router on the output side;
  	empty_router = new EventRouter();
  	router_ = empty_router;};
  	~CollectiveConnector(){if(empty_router != NULL) delete empty_router;}
  	void spatialNegotiation ();
  private:
  	Subconnector* makeSubconnector (int remoteRank);
  	void addRoutingInterval(IndexInterval i, Subconnector* subconn);
    };
 class MessageConnector : virtual public Connector {
 };
  class MessageOutputConnector : public OutputConnector,
  	  	  	  	  	  	  	  	  public MessageConnector,
  	  	  	  	  	  	  	  	  public PostCommunicationConnector {
  private:
 //   OutputSynchronizer synch;
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
    void addRoutingInterval (IndexInterval i, Subconnector* subconn);
    void postCommunication ();
  };
  
  class MessageInputConnector : public InputConnector, public MessageConnector {
    
  private:
  //  InputSynchronizer synch;
    MessageHandler* handleMessage_;
    Index::Type type_;
  public:
    MessageInputConnector (ConnectorInfo connInfo,
			   SpatialInputNegotiator* spatialNegotiator,
			   MessageHandler* handleMessage,
			   Index::Type type,
			   MPI::Intracomm comm);
    Subconnector* makeSubconnector (int remoteRank);
  };
  
}
#endif
#define MUSIC_CONNECTOR_HH
#endif

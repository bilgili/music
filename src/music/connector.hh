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
    virtual void specialize (Clock& localTime) { }

    std::string receiverAppName () const { return info.receiverAppName (); }
    std::string receiverPortName () const { return info.receiverPortName (); }
    int receiverPortCode () const { return info.receiverPortCode (); }
    int remoteLeader () const { return info.remoteLeader (); }
    int width(){return spatialNegotiator_->getWidth();}
    int maxLocalWidth () { return spatialNegotiator_->maxLocalWidth (); }
    bool isLeader ();
    virtual void setInterpolate(bool val){interpolate_ =  val;}
    virtual void setLatency( ClockState latency){ latency_ = latency;}
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
    virtual Subconnector* makeSubconnector (int remoteRank) =0;
  };


  class PostCommunicationConnector : virtual public Connector {
  public:
    virtual void postCommunication () = 0;
    virtual Subconnector* makeSubconnector (int remoteRank) {return NULL;};
  };
  class PreCommunicationConnector : virtual public Connector {
    public:
      virtual void preCommunication () = 0;
      virtual Subconnector* makeSubconnector (int remoteRank) {return NULL;};
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
      : sampler_ (sampler), type_ (type),localTime_(NULL) { }
    ClockState remoteTickInterval (ClockState tickInterval);
    MPI::Datatype getDataType(){return type_;}
    virtual void initialize ();
  protected:
    virtual void initialCommunication()=0;
    friend class SpecializedContConnector;
  };  

  class ContOutputConnector : public ContConnector, public OutputConnector, public PreCommunicationConnector  {
  protected:
    Distributor distributor_;
    PreCommunicationConnector* connector; //specialized connector
    ContOutputConnector(Sampler& sampler, MPI::Datatype type);
  public:
    ContOutputConnector (ConnectorInfo connInfo,
			 SpatialNegotiator* spatialNegotiator,
			 MPI::Intracomm comm,
			 Sampler& sampler,
			 MPI::Datatype type);
    ~ContOutputConnector();
    Subconnector* makeSubconnector (int remoteRank);
    void addRoutingInterval (IndexInterval i, Subconnector* osubconn);
    void specialize (Clock& localTime);
    void preCommunication () {connector->preCommunication(); }
    void initialize (){connector->initialize();ContConnector::initialize();};
  protected:
    void initialCommunication();
    friend class SpecializedContOuputConnector;
  };
  
  class ContInputConnector : public ContConnector,
  public InputConnector,
  public PostCommunicationConnector {

	  protected:
	  Collector collector_;
	  double delay_;
	  PostCommunicationConnector* connector;
	  bool divisibleDelay (Clock& localTime);
	  ContInputConnector(Sampler& sampler,MPI::Datatype type,	double delay);
	  public:
	  ContInputConnector (ConnectorInfo connInfo,
			  SpatialNegotiator* spatialNegotiator,
			  MPI::Intracomm comm,
			  Sampler& sampler,
			  MPI::Datatype type,
			  double delay);
	  ~ContInputConnector();
	  Subconnector* makeSubconnector (int remoteRank);
	  void addRoutingInterval (IndexInterval i, Subconnector* isubconn);
	  void specialize (Clock& localTime);

	  void postCommunication () {connector->postCommunication(); }
	  void initialize (){connector->initialize();ContConnector::initialize();};
	  protected:
	  void initialCommunication();
	  private:
	  int initialBufferedTicks();
	  friend class SpecializedContInputConnector;
  };
  


  class EventConnector : virtual public Connector {
    };

  class EventOutputConnector : public OutputConnector, public EventConnector {
  private:
	//OutputSynchronizer synch;
    EventRoutingMap<FIBO*>* routingMap_;
  protected:
    EventOutputConnector():routingMap_(NULL){};
    EventOutputConnector(EventRoutingMap<FIBO*>* routingMap): routingMap_(routingMap){};
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
  class EventInputConnector : public InputConnector, public EventConnector {
  protected:
	//InputSynchronizer synch;
//	EventRoutingMap<EventHandlerGlobalIndex*>* routingMap_;
    EventHandlerPtr handleEvent_;
    Index::Type type_;
  protected:
    EventInputConnector():type_( Index::UNDEFINED){};
    EventInputConnector (Index::Type type,  EventHandlerPtr handleEvent): handleEvent_(handleEvent),type_(type){};
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
     * that is currently done in CollectiveConnector::addRoutingInterval.
     * This could give an opportunity to have different event handlers for different ranges of indexes as in collective algorithm.
     * Do we need this? If not, then it's better to leave as it is.
     */
    void addRoutingInterval (IndexInterval i, Subconnector* subconn);
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
  /* remedius
   * New class hierarchy CollectiveConnector was introduced in order to create CollectiveSubconnector object.
   * The difference between InputConnector, OutputConnector instances and  CollectiveConnector is
   * the latest creates one CollectiveSubconnector responsible for the collective communication
   * among all ranks on the output and input sides, while InputConnector and OutputConnector
   * instances are responsible for creating as much Subconnectors as necessary for each point2point connectivity
   * for output and input sides.
   */
  class CollectiveConnector: public virtual  Connector{
  protected:
	  bool high_;
	  Subconnector*subconnector;
	  MPI::Intracomm intracomm_;
	  CollectiveConnector(bool high);
	  virtual Subconnector* makeSubconnector (void *param)=0;
  public:
	  void createIntercomm ();
	  void freeIntercomm ();
  };
  class ContCollectiveConnector: public CollectiveConnector{
	  MPI::Datatype data_type_;
  protected:
	  ContCollectiveConnector( MPI::Datatype type, bool high);
  public:
	  Subconnector* makeSubconnector (void *param);
  };

  class EventCollectiveConnector: public CollectiveConnector{
  protected:
	  EventRoutingMap<EventHandlerGlobalIndex*>* routingMap_input; //is used to fill the information for the receiver
	  EventRouter *router_; //is used for processing received information
	  EventCollectiveConnector( bool high);
  public:
	  Subconnector* makeSubconnector (void *param);
  };

  class EventInputCollectiveConnector:  public EventInputConnector, public EventCollectiveConnector {
  public:
	  EventInputCollectiveConnector(ConnectorInfo connInfo,
			  SpatialNegotiator* spatialNegotiator,
			  MPI::Intracomm comm,
			  Index::Type type,
			  EventHandlerPtr handleEvent);
	  ~EventInputCollectiveConnector();
	  void spatialNegotiation ();
  private:
	  void addRoutingInterval(IndexInterval i, Subconnector* subconn);
  };
  class EventOutputCollectiveConnector:  public EventOutputConnector,public EventCollectiveConnector{
  public:
	  EventOutputCollectiveConnector(ConnectorInfo connInfo,
			  SpatialNegotiator* spatialNegotiator,
			  MPI::Intracomm comm,
			  EventRoutingMap<FIBO *>* routingMap);
	  ~EventOutputCollectiveConnector();
	  void spatialNegotiation ();
  };

  class ContInputCollectiveConnector: public ContInputConnector, public ContCollectiveConnector
  {
  public:
	  ContInputCollectiveConnector(ConnectorInfo connInfo,
			  SpatialNegotiator* spatialNegotiator,
			  MPI::Intracomm comm,
			  Sampler& sampler,
			  MPI::Datatype type,
			  double delay);
	  void spatialNegotiation ();
  private:
	  void receiveRemoteCommRankID(std::map<int,int> &remoteToCollectiveRankMap);
  };

  class ContOutputCollectiveConnector:  public ContOutputConnector, public ContCollectiveConnector
  {
  public:
	  ContOutputCollectiveConnector(ConnectorInfo connInfo,
			  SpatialNegotiator* spatialNegotiator,
			  MPI::Intracomm comm,
			  Sampler& sampler,
			  MPI::Datatype type);
	  void spatialNegotiation ();
  private:
	  void sendLocalCommRankID();
  };
  class SpecializedContConnector{
  protected:
	  Sampler& sampler_;
	  Clock *localTime_;
	  Clock &remoteTime_;
	  ClockState &latency;
	  bool &interp;
	  SpecializedContConnector(ContConnector *conn):
	  sampler_ ( conn->sampler_),
	  localTime_(conn->localTime_),
	  remoteTime_(conn->remoteTime_),
	  latency( conn->latency_),
	  interp( conn->interpolate_){};
  };
  class SpecializedContOuputConnector:public SpecializedContConnector{
  protected:
	  Distributor &distributor_;
	  SpecializedContOuputConnector( ContOutputConnector *conn):
		  SpecializedContConnector(conn),
		  distributor_(conn->distributor_){ }
  };
  class SpecializedContInputConnector:public SpecializedContConnector{
  protected:
	  Collector &collector_;
	  SpecializedContInputConnector( ContInputConnector *conn):
		  SpecializedContConnector(conn),
		  collector_(conn->collector_){}
  };
  class PlainContOutputConnector : public PreCommunicationConnector, public SpecializedContOuputConnector {
    public:
      PlainContOutputConnector ( ContOutputConnector *conn):SpecializedContOuputConnector(conn){};
      void initialize ();
      void preCommunication();
    private:
      bool sample();

    };

    class InterpolatingContOutputConnector :  public PreCommunicationConnector, public SpecializedContOuputConnector
  	{
    public:
      InterpolatingContOutputConnector (ContOutputConnector *conn):SpecializedContOuputConnector(conn){};
      void initialize ();
      void preCommunication();
    private:
      bool interpolate();
      bool sample();
      double interpolationCoefficient();

    };
    class PlainContInputConnector : public PostCommunicationConnector, public SpecializedContInputConnector {
     public:
       PlainContInputConnector (ContInputConnector *conn):SpecializedContInputConnector(conn){};
       void initialize ();
       void postCommunication ();
     };

     class InterpolatingContInputConnector : public PostCommunicationConnector, public SpecializedContInputConnector  {
       bool first_;
     public:
       InterpolatingContInputConnector (ContInputConnector *conn):SpecializedContInputConnector(conn){};
       void initialize ();
       void postCommunication ();
     private:
       bool sample();
       double interpolationCoefficient();
     };
}
#endif
#define MUSIC_CONNECTOR_HH
#endif

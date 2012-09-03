/*
 *  This file is part of MUSIC.
 *  Copyright (C) 2007, 2008, 2009, 2012 INCF
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

#include "music/music-config.hh"

#if MUSIC_USE_MPI

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
#include <music/event_routing_map.hh>
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

  class Connector
  {
  protected:
    ConnectorInfo info;
    IndexMap* indices_;
    Index::Type type_;
    MPI::Intracomm comm;
    MPI::Intercomm intercomm;
    std::vector<Subconnector*> rsubconn;
    ClockState latency_;
    // interpolate rather than picking value closest in time
    bool interpolate_;
    int width_;
    int maxLocalWidth_;
    unsigned int idFlag_;

    Connector () { };
    Connector (ConnectorInfo info_,
	       IndexMap* indices,
	       Index::Type type,
	       MPI::Intracomm c);
    Connector (ConnectorInfo info_,
	       IndexMap* indices,
	       Index::Type type,
	       MPI::Intracomm c,
	       MPI::Intercomm ic);
  public:
    void report ()
    {
      for (std::vector<Subconnector*>::iterator subconnector = rsubconn.begin ();
	   subconnector != rsubconn.end ();
	   ++subconnector)
	(*subconnector)->report ();
    }
    virtual ~Connector ()
    {
      for (std::vector<Subconnector*>::iterator subconnector = rsubconn.begin ();
	   subconnector != rsubconn.end ();
	   ++subconnector)
	delete *subconnector;
    }
    virtual void specialize (Clock& localTime) { }

    unsigned int idFlag () const { return idFlag_; }
    std::string receiverAppName () const { return info.receiverAppName (); }
    std::string receiverPortName () const { return info.receiverPortName (); }
    int receiverPortCode () const { return info.receiverPortCode (); }
    int remoteLeader () const { return info.remoteLeader (); }
    int width(){return width_;}
    int maxLocalWidth () { return maxLocalWidth_; }
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
  protected:
    virtual void addRoutingInterval (IndexInterval i, Subconnector* s){};
    virtual Subconnector* makeSubconnector (int remoteRank){return NULL;};

    virtual void spatialNegotiation (SpatialNegotiator *spatialNegotiator_);
    virtual SpatialNegotiator *createSpatialNegotiator(){return NULL;};
  };


  class ProxyConnector : virtual public Connector {
    int senderLeader_;
    int senderNProcs_;
    int receiverLeader_;
    int receiverNProcs_;
  public:
    ProxyConnector (int senderLeader, int senderNProcs,
		    int receiverLeader, int receiverNProcs)
      : senderLeader_ (senderLeader), senderNProcs_ (senderNProcs),
	receiverLeader_ (receiverLeader), receiverNProcs_ (receiverNProcs)
    { }
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
  protected:
    OutputConnector(){};
    SpatialNegotiator *createSpatialNegotiator();
  };

  class InputConnector : virtual public Connector {
  public:
  protected:
    InputConnector(){};
    SpatialNegotiator *createSpatialNegotiator();
  };


  class ContConnector : virtual public Connector {
  protected:
    Sampler& sampler_;
    MPI::Datatype  data_type_;

    Clock *localTime_;
    Clock remoteTime_;

  public:
    ContConnector (Sampler& sampler, MPI::Datatype type)
      : sampler_ (sampler),  data_type_ (type),localTime_(NULL) { }
    ClockState remoteTickInterval (ClockState tickInterval);
    MPI::Datatype getDataType(){return  data_type_;}
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
			 IndexMap* indices,
			 Index::Type type,
			 MPI::Intracomm comm,
			 Sampler& sampler,
			 MPI::Datatype data_type);
    ~ContOutputConnector();

    void specialize (Clock& localTime);
    void preCommunication () {connector->preCommunication(); }
    void initialize (){connector->initialize();ContConnector::initialize();};
  protected:
    void addRoutingInterval (IndexInterval i, Subconnector* osubconn);
    Subconnector* makeSubconnector (int remoteRank);
    void initialCommunication();
    friend class SpecializedContOuputConnector;
  };

  class ContInputConnector : public ContConnector,  public InputConnector,  public PostCommunicationConnector {

  protected:
    Collector collector_;
    double delay_;
    PostCommunicationConnector* connector;
    bool divisibleDelay (Clock& localTime);
    ContInputConnector(Sampler& sampler,MPI::Datatype type,	double delay);
  public:
    ContInputConnector (ConnectorInfo connInfo,
			IndexMap* indices,
			Index::Type type,
			MPI::Intracomm comm,
			Sampler& sampler,
			MPI::Datatype data_type,
			double delay);
    ~ContInputConnector();

    void specialize (Clock& localTime);

    void postCommunication () {connector->postCommunication(); }
    void initialize (){connector->initialize();ContConnector::initialize();};
  protected:
    void addRoutingInterval (IndexInterval i, Subconnector* isubconn);
    void initialCommunication();
    Subconnector* makeSubconnector (int remoteRank);
  private:
    int initialBufferedTicks();
    friend class SpecializedContInputConnector;
  };


  class EventOutputConnector : public OutputConnector{
  private:
    EventRoutingMap<FIBO*>* routingMap_;
  public:
    EventOutputConnector (ConnectorInfo connInfo,
			  IndexMap* indices,
			  Index::Type type,
			  MPI::Intracomm comm,
			  EventRoutingMap<FIBO*>* routingMap): Connector(connInfo, indices, type ,comm),
							       routingMap_(routingMap){
    };
    ~EventOutputConnector(){}
  protected:
    EventOutputConnector():routingMap_(NULL){};
    EventOutputConnector(EventRoutingMap<FIBO*>* routingMap): routingMap_(routingMap){};
    Subconnector* makeSubconnector (int remoteRank);
    void addRoutingInterval (IndexInterval i, Subconnector* subconn);
  };
  class EventInputConnector : public InputConnector {
  protected:
    //	EventRoutingMap<EventHandlerGlobalIndex*>* routingMap_;
    EventHandlerPtr handleEvent_;
  public:

    EventInputConnector (ConnectorInfo connInfo,
			 IndexMap* indices,
			 Index::Type type,
			 MPI::Intracomm comm,
			 EventHandlerPtr handleEvent):Connector(connInfo, indices, type ,comm),
						      handleEvent_(handleEvent){};
    ~EventInputConnector(){}
  protected:
    EventInputConnector(){};
    EventInputConnector (EventHandlerPtr handleEvent): handleEvent_(handleEvent){};
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


  class MessageOutputConnector : public OutputConnector,
				 public PostCommunicationConnector {
  private:
    //   OutputSynchronizer synch;
    FIBO buffer;
    bool bufferAdded;
    std::vector<FIBO*>& buffers_;
    void send ();
  public:
    MessageOutputConnector (ConnectorInfo connInfo,
			    IndexMap* indices,
			    Index::Type type,
			    MPI::Intracomm comm,
			    std::vector<FIBO*>& buffers);

    void postCommunication ();
  protected:
    void addRoutingInterval (IndexInterval i, Subconnector* subconn);
    Subconnector* makeSubconnector (int remoteRank);
    SpatialNegotiator *createSpatialNegotiator() {return OutputConnector::createSpatialNegotiator();};
  };

  class MessageInputConnector : public InputConnector {

  private:
    MessageHandler* handleMessage_;
  public:
    MessageInputConnector (ConnectorInfo connInfo,
			   IndexMap* indices,
			   Index::Type type,
			   MessageHandler* handleMessage,
			   MPI::Intracomm comm);
  protected:
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

  class MultiConnector {
    std::vector<Connector*> connectors_;
  public:
    void add (Connector* connector) { connectors_.push_back (connector); }
    void initialize () { }
    void tick ()
    {
      for (std::vector<Connector*>::iterator c = connectors_.begin ();
	   c != connectors_.end ();
	   ++c)
	(*c)->tick ();
    }
  };

  class CollectiveConnector: virtual public Connector {
  private:
    static unsigned int nextFlag_;
    static unsigned int makeFlag ()
    {
      unsigned int flag = nextFlag_;
      nextFlag_ <<= 1;
      return flag;
    }
  protected:
    bool high_;
    Subconnector*subconnector;
    MPI::Intracomm intracomm_;
    CollectiveConnector (bool high);
    virtual Subconnector* makeSubconnector (void *param)=0;
  public:
    void createIntercomm ();
    void freeIntercomm ();
  };

  class ContCollectiveConnector: public CollectiveConnector{
    MPI::Datatype data_type;
  protected:
    ContCollectiveConnector( MPI::Datatype type, bool high);
    Subconnector* makeSubconnector (void *param);
  };

  class EventCollectiveConnector: public CollectiveConnector{
  protected:
    EventRouter *router_; //is used for processing received information
    EventCollectiveConnector( bool high);
    Subconnector* makeSubconnector (void *param);
  };

  class EventInputCollectiveConnector:  public EventInputConnector, public EventCollectiveConnector {
    EventRoutingMap<EventHandlerPtr*>* routingMap_input; //is used to fill the information for the receiver
  public:
    EventInputCollectiveConnector(ConnectorInfo connInfo,
				  IndexMap* indices,
				  Index::Type type,
				  MPI::Intracomm comm,
				  EventHandlerPtr handleEvent);
    ~EventInputCollectiveConnector();
  protected:
    void spatialNegotiation ( SpatialNegotiator* spatialNegotiator_);
    void addRoutingInterval(IndexInterval i, Subconnector* subconn);
  };

  class EventOutputCollectiveConnector:  public EventOutputConnector,public EventCollectiveConnector{
  public:
    EventOutputCollectiveConnector(ConnectorInfo connInfo,
				   IndexMap* indices,
				   Index::Type type,
				   MPI::Intracomm comm,
				   EventRoutingMap<FIBO *>* routingMap);
    ~EventOutputCollectiveConnector();
  protected:
    void spatialNegotiation ( SpatialNegotiator* spatialNegotiator_);
  };

  class ContInputCollectiveConnector: public ContInputConnector, public ContCollectiveConnector
  {
  public:
    ContInputCollectiveConnector(ConnectorInfo connInfo,
				 IndexMap* indices,
				 Index::Type type,
				 MPI::Intracomm comm,
				 Sampler& sampler,
				 MPI::Datatype data_type,
				 double delay);
  protected:
    void spatialNegotiation ( SpatialNegotiator* spatialNegotiator_);
  private:
    void receiveRemoteCommRankID(std::map<int,int> &remoteToCollectiveRankMap);
  };

  class ContOutputCollectiveConnector:  public ContOutputConnector, public ContCollectiveConnector
  {
  public:
    ContOutputCollectiveConnector(ConnectorInfo connInfo,
				  IndexMap* indices,
				  Index::Type type,
				  MPI::Intracomm comm,
				  Sampler& sampler,
				  MPI::Datatype data_type);
  protected:
    void spatialNegotiation ( SpatialNegotiator* spatialNegotiator_);
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
  protected:
    PlainContOutputConnector ( ContOutputConnector *conn):SpecializedContOuputConnector(conn){};
  public:
    void initialize ();
    void preCommunication();
  private:
    bool sample();
    friend class ContOutputConnector;

  };

  class InterpolatingContOutputConnector :  public PreCommunicationConnector, public SpecializedContOuputConnector
  {
  protected:
    InterpolatingContOutputConnector (ContOutputConnector *conn):SpecializedContOuputConnector(conn){};
  public:
    void initialize ();
    void preCommunication();
  private:
    bool interpolate();
    bool sample();
    double interpolationCoefficient();
    friend class ContOutputConnector;

  };
  class PlainContInputConnector : public PostCommunicationConnector, public SpecializedContInputConnector {
  protected:
    PlainContInputConnector (ContInputConnector *conn):SpecializedContInputConnector(conn){};
  public:
    void initialize ();
    void postCommunication ();
    friend class ContInputConnector;
  };

  class InterpolatingContInputConnector : public PostCommunicationConnector, public SpecializedContInputConnector  {
    bool first_;
  protected:
    InterpolatingContInputConnector (ContInputConnector *conn):SpecializedContInputConnector(conn){};
  public:
    void initialize ();
    void postCommunication ();
  private:
    bool sample();
    double interpolationCoefficient();
    friend class ContInputConnector;
  };
}
#endif
#define MUSIC_CONNECTOR_HH
#endif

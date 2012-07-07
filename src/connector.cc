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
#include "music/connector.hh"
#ifdef USE_MPI
// connector.hh needs to be included first since it causes inclusion
// of mpi.h (in data_map.hh).  mpi.h must be included before other
// header files on BG/L

#include "music/error.hh"
#include "music/communication.hh"
#include <cmath>
namespace MUSIC {

  Connector::Connector (ConnectorInfo info_,
			SpatialNegotiator* spatialNegotiator,
			MPI::Intracomm c)
    : info (info_),
      spatialNegotiator_ (spatialNegotiator),
      comm (c)
  {
  }


  Connector::Connector (ConnectorInfo info_,
			SpatialNegotiator* spatialNegotiator,
			MPI::Intracomm c,
			MPI::Intercomm ic)
    : info (info_),
      spatialNegotiator_ (spatialNegotiator),
      comm (c),
      intercomm (ic)
  {
  }


  bool
  Connector::isLeader ()
  {
    return comm.Get_rank () == 0;
  }

  
  void
  Connector::createIntercomm ()
  {
    intercomm = comm.Create_intercomm (0,
				       MPI::COMM_WORLD,
				       info.remoteLeader (),
				       CREATE_INTERCOMM_MSG);
  }


  void
  Connector::freeIntercomm ()
  {
    intercomm.Free ();
  }
  void Connector::spatialNegotiation ()
    {
      std::map<int, Subconnector*> subconnectors;
      for (NegotiationIterator i
  	   = spatialNegotiator_->negotiate (comm,
  					    intercomm,
  					    info.nProcesses (),
  					    this); // only for debugging
  	 !i.end ();
  	 ++i)
        {
  	std::map<int, Subconnector*>::iterator c
  	  = subconnectors.find (i->rank ());
  	Subconnector* subconn;
  	if (c != subconnectors.end ())
  	  subconn = c->second;
  	else
  	  {
  	    subconn = makeSubconnector (i->rank ());
  	    subconnectors.insert (std::make_pair (i->rank (), subconn));
  	    rsubconn.push_back (subconn);
  	  }
  	/*MUSIC_LOG (MPI::COMM_WORLD.Get_rank ()
  		   << ": ("
  		   << i->begin () << ", "
  		   << i->end () << ", "
  		   << i->local () << ") -> " << i->rank ());*/
  	addRoutingInterval (i->interval (), subconn);
        }
    }
  /* remedius
   * lessSubconnector function was moved from runtime.cc since
   * Runtime object doesn't contain vector of subconnectors anymore.
   */
  bool
   lessSubconnector (const Subconnector* c1, const Subconnector* c2)
   {
     return (c1->remoteWorldRank () < c2->remoteWorldRank ()
  	    || (c1->remoteWorldRank () == c2->remoteWorldRank ()
  		&& c1->receiverPortCode () < c2->receiverPortCode ()));
   }
  void Connector::initialize(){
	  sort (rsubconn.begin (), rsubconn.end (),  lessSubconnector);
 }

  bool Connector::finalizeSimulation(){
	  bool dataStillFlowing = false;
	  for (std::vector<Subconnector*>::iterator s = rsubconn.begin (); 	 s != rsubconn.end ();  	 ++s)
	  	        (*s)->flush(dataStillFlowing);
	  return !dataStillFlowing;
  }
  void Connector::tick(){
	  for (std::vector<Subconnector*>::iterator s = rsubconn.begin ();	 s != rsubconn.end ();
	 	  	  	 ++s){
	 	  	        (*s)->maybeCommunicate();

	  }
  }

  /********************************************************************
   *
   * Collective Connector
   *
   ********************************************************************/
  void CollectiveConnector::spatialNegotiation(){
	  Subconnector *subconn = makeSubconnector(-1);
	  rsubconn.push_back (subconn);
	  for (NegotiationIterator i
	    	   = spatialNegotiator_->negotiateSimple (comm); // only for debugging
	    	 !i.end ();
	    	 ++i)
	          {

		  		  addRoutingInterval (i->interval (), subconn);
	  	  }
  }
  Subconnector*
  CollectiveConnector::makeSubconnector(int remoteRank)
  {
	  if(subconnector == NULL){
		  subconnector = new CollectiveSubconnector(//synchronizer(),
				  intercomm.Merge(high),router_);
	  }
	  return subconnector;
  }
  void
  CollectiveConnector::addRoutingInterval(IndexInterval i, Subconnector* subconn)
  {
	  if(routingMap_input != NULL) {// if we are on the input side, we have to insert an interval since addRoutingInterval
		                      // of the EventInputConnector does nothing.
		 // routingMap_input-> insert (i, ( type_ == Index::GLOBAL ? handleEvent_.global() : handleEvent_.local() ));
		  routingMap_input-> insert (i, ( handleEvent_.global() ));
	  }
	  else{
		  OutputSubconnector*osubconn= dynamic_cast<OutputSubconnector*>(subconn);
		  routingMap_output->insert (i, osubconn->buffer ());
	  }
  }



  /********************************************************************
   *
   * Cont Connectors
   *
   ********************************************************************/
  void ContConnector::remoteTick(){
	  remoteTime_.tick();
  }
  ClockState
  ContConnector::remoteTickInterval (ClockState tickInterval)
  {
    ClockState::Serialized sRemoteTickInterval;
    if (isLeader ())
      {
	// exchange tickInterval with peer leader
	sRemoteTickInterval = tickInterval.serialize ();
	intercomm.Sendrecv_replace (&sRemoteTickInterval, 2, MPI::UNSIGNED_LONG,
				    0, TICKINTERVAL_MSG,
				    0, TICKINTERVAL_MSG);
      }
    // broadcast to peers
    comm.Bcast (&sRemoteTickInterval, 2, MPI::UNSIGNED_LONG, 0);
    return sRemoteTickInterval.deserialize ();
  }

  ContOutputConnector::ContOutputConnector (ConnectorInfo connInfo,
					    SpatialNegotiator* spatialNegotiator,
					    MPI::Intracomm comm,
					    Sampler& sampler,
					    MPI::Datatype type)
    : Connector (connInfo, spatialNegotiator, comm),
      ContConnector (sampler, type)
  {
  }

  
  Subconnector*
  ContOutputConnector::makeSubconnector (int remoteRank)
  {
    return new ContOutputSubconnector (//synchronizer (),
				       intercomm,
				       remoteLeader (),
				       remoteRank,
				       receiverPortCode (),
				       type_);
  }
  

  Connector*
  ContOutputConnector::specialize (Clock& localTime)
  {
    Connector* connector;
    ClockState tickInterval = localTime.tickInterval ();
    ClockState rTickInterval = remoteTickInterval (tickInterval);
    localTime_ = &localTime;
    remoteTime_.configure (localTime_->timebase (), rTickInterval);
    if (tickInterval < rTickInterval)
      connector = new InterpolatingContOutputConnector (*this);
    else
      connector = new PlainContOutputConnector (*this);

    delete this; // delete ourselves!
    return connector;
  }

  
  void
  ContOutputConnector::addRoutingInterval (IndexInterval i,
					   Subconnector* subconn)
  {
	  OutputSubconnector*osubconn= dynamic_cast<OutputSubconnector*>(subconn);
    distributor_.addRoutingInterval (i, osubconn->buffer ());
  }
  void ContOutputConnector::initialCommunication()
   {
 	  for (std::vector<Subconnector*>::iterator s = rsubconn.begin (); s != rsubconn.end (); ++s)
 	 	        (*s)->initialCommunication (0.0);
   }
  
  PlainContOutputConnector::PlainContOutputConnector
  (ContOutputConnector& connector)
    : Connector (connector),
      ContOutputConnector (connector)
  {
  }


  void
  PlainContOutputConnector::initialize ()
  {
    distributor_.configure (sampler_.dataMap ());
    distributor_.initialize ();
   // synch.initialize ();

    // put one element in send buffers
    distributor_.distribute ();
    ContConnector::initialize();
  }


  void
  PlainContOutputConnector::preCommunication ()
  {
	  if (sample ())
	  {
		  // copy application data to send buffers
		  distributor_.distribute ();
	  }
  }
  // Start sampling (and fill the output buffers) at a time dependent
  // on latency and receiver's tick interval.  A negative latency can
  // delay start of sampling beyond time 0.  The tickInterval together
  // with the strict comparison has the purpose of supplying an
  // interpolating receiver side with samples.
  bool
   PlainContOutputConnector::sample (){
	  return (localTime_->integerTime () + latency_ + remoteTime_.tickInterval ()
	  	    > 0);
  }

  InterpolatingContOutputConnector::InterpolatingContOutputConnector
  (ContOutputConnector& connector)
    : Connector (connector),
      ContOutputConnector (connector)
  {
  }


  void
  InterpolatingContOutputConnector::initialize ()
  {
	  // Set the remoteTime which is used to control sampling and
	  // interpolation.
	  //
	  // For positive latencies, the integer part (in terms of receiver
	  // ticks) of the latency is handled by filling up the receiver
	  // buffers using InputSynchronizer::initialBufferedTicks ().
	  // remoteTime then holds the fractional part.
	  if (latency_ > 0)
	  {
		  ClockState startTime = - latency_ % remoteTime_.tickInterval ();
		  if (latency_ >= localTime_->tickInterval ())
			  startTime = startTime + remoteTime_.tickInterval ();
		  remoteTime_.set (startTime);
	  }
	  else
		  remoteTime_.set (- latency_);
	  distributor_.configure (sampler_.interpolationDataMap ());
	  distributor_.initialize ();
	  // synch.initialize ();

	  // put one element in send buffers
	  sampler_.sample ();
	  sampler_.interpolate (1.0);
	  distributor_.distribute ();
	  ContConnector::initialize();
  }
  // After the last sample at 1 localTime will be between remoteTime
  // and remoteTime + localTime->tickInterval.  We trigger on this
  // situation and forward remoteTime.
  bool
  InterpolatingContOutputConnector::sample()
  {
	  ClockState sampleWindowLow
	  = remoteTime_.integerTime () - localTime_->tickInterval ();
	  ClockState sampleWindowHigh
	  = remoteTime_.integerTime () + localTime_->tickInterval ();
	  return (sampleWindowLow <= localTime_->integerTime ()
			  && localTime_->integerTime () < sampleWindowHigh);
  }
  bool
  InterpolatingContOutputConnector::interpolate()
  {
	  ClockState sampleWindowHigh
	  = remoteTime_.integerTime () + localTime_->tickInterval ();
	  return (remoteTime_.integerTime () <= localTime_->integerTime ()
			  && localTime_->integerTime () < sampleWindowHigh);
  }
  double
  InterpolatingContOutputConnector::interpolationCoefficient()
  {
	  ClockState prevSampleTime
	  = localTime_->integerTime () - localTime_->tickInterval ();
	  double c = ((double) (remoteTime_.integerTime () - prevSampleTime)
			  / (double) localTime_->tickInterval ());

	  MUSIC_LOGR ("interpolationCoefficient = " << c);
	  // NOTE: preliminary implementation which just provides
	  // the functionality specified in the API
	  if (interpolate_)
		  return c;
	  else
		  return round (c);
  }

  void
  InterpolatingContOutputConnector::preCommunication ()
  {
	  if (sample ()){
		  // sampling before and after time of receiver tick
		  sampler_.sampleOnce ();
	  }
	  if (interpolate ())
	  {
		  sampler_.interpolate (interpolationCoefficient ());
		  remoteTick ();
		  distributor_.distribute ();
	  }
  }


  ContInputConnector::ContInputConnector (ConnectorInfo connInfo,
					  SpatialNegotiator* spatialNegotiator,
					  MPI::Intracomm comm,
					  Sampler& sampler,
					  MPI::Datatype type,
					  double delay)
    : Connector (connInfo, spatialNegotiator, comm),
      ContConnector (sampler, type),
      delay_ (delay)
  {
  }

  
  Subconnector*
  ContInputConnector::makeSubconnector (int remoteRank)
  {
	  int receiverRank = intercomm.Get_rank ();
    return new ContInputSubconnector (//synchronizer (),
				      intercomm,
				      remoteLeader (),
				      remoteRank,
				      receiverRank,
				      receiverPortCode (),
				      type_);
  }


  bool
  ContInputConnector::divisibleDelay (Clock& localTime)
  {
    ClockState delay (delay_, localTime.timebase ());
    return (delay % localTime.tickInterval ()) == 0;
  }
  

  Connector*
  ContInputConnector::specialize (Clock& localTime)
  {
    Connector* connector;
    ClockState tickInterval = localTime.tickInterval ();
    ClockState rTickInterval = remoteTickInterval (tickInterval);
    localTime_ = &localTime;
    remoteTime_.configure (localTime_->timebase (), rTickInterval);

    if (tickInterval < rTickInterval
	|| (tickInterval == rTickInterval
	    && !divisibleDelay (localTime)))
      connector = new InterpolatingContInputConnector (*this);
    else
      connector = new PlainContInputConnector (*this);

    delete this; // delete ourselves!
    return connector;
  }
  void
  ContInputConnector::initialCommunication()
  {
	  double initialBTicks = initialBufferedTicks();
	  for (std::vector<Subconnector*>::iterator s = rsubconn.begin (); s != rsubconn.end (); ++s)
	  	 	        (*s)->initialCommunication (initialBTicks);
  }
  // Return the number of copies of the data sampled by the sender
  // Runtime constructor which should be stored in the receiver
  // buffers at the first tick () (which occurs at the end of the
  // Runtime constructor)
  int
  ContInputConnector::initialBufferedTicks(){
	    if (remoteTime_.tickInterval () < localTime_->tickInterval ())
	      {
		// InterpolatingOutputConnector - PlainInputConnector

		if (delay_ <= 0)
		  return 0;
		else
		  {
		    // Need to add a sample first when we pass the receiver
		    // tick (=> - 1).  If we haven't passed, the interpolator
		    // could simply use an interpolation coefficient of 0.0.
		    // (But this will never happen since that case isn't
		    // handled by an InterpolatingOutputConnector.)
		    int ticks = (delay_ - 1) / localTime_->tickInterval ();

		    // Need to add a sample if we go outside of the sender
		    // interpolation window
		    if (delay_ >= remoteTime_.tickInterval ())
		      ticks += 1;

		    return ticks;
		  }
	      }
	    else
	      {
		// PlainOutputConnector - InterpolatingInputConnector

		if (delay_ <= 0)
		  return 0;
		else
		  // Need to add a sample first when we pass the receiver
		  // tick (=> - 1).  If we haven't passed, the interpolator
		  // can simply use an interpolation coefficient of 0.0.
		  return 1 + (delay_ - 1) / localTime_->tickInterval ();
	      }
  }
  void
  ContInputConnector::addRoutingInterval (IndexInterval i,
					  Subconnector* subconn)
  {
	  InputSubconnector*isubconn= dynamic_cast<InputSubconnector*>(subconn);
    collector_.addRoutingInterval (i, isubconn->buffer ());
  }
  
  
  PlainContInputConnector::PlainContInputConnector
  (ContInputConnector& connector)
    :  Connector (connector),
       ContInputConnector (connector)
  {
  }


  void
  PlainContInputConnector::initialize ()
  {
   // collector_.configure (sampler_.dataMap (), synch.allowedBuffered () + 1);
	collector_.configure (sampler_.dataMap (),  CONT_BUFFER_MAX);
    collector_.initialize ();
    ContConnector::initialize();

    //synch.initialize ();
  }
  




  void
  PlainContInputConnector::postCommunication ()
  {
    // collect data from input buffers and write to application
    collector_.collect ();
  }


  InterpolatingContInputConnector::InterpolatingContInputConnector
  (ContInputConnector& connector)
    : Connector (connector),
      ContInputConnector (connector),
      first_ (true)
  {
  }


  void
  InterpolatingContInputConnector::initialize ()
  {
	  if (latency_ > 0)
		  remoteTime_.set (latency_ % remoteTime_.tickInterval ()
				  - 2 * remoteTime_.tickInterval ());
	  else
		  remoteTime_.set (latency_ % remoteTime_.tickInterval ()
				  - remoteTime_.tickInterval ());
  //  collector_.configure (sampler_.interpolationDataMap (),
//			  synch.allowedBuffered () + 1);
	collector_.configure (sampler_.interpolationDataMap (), CONT_BUFFER_MAX);
    collector_.initialize ();
    ContConnector::initialize();
   // synch.initialize ();
  }


  bool
  InterpolatingContInputConnector::sample ()
  {
    return localTime_->integerTime () > remoteTime_.integerTime ();
  }


  double
  InterpolatingContInputConnector::interpolationCoefficient ()
  {
    ClockState prevSampleTime
      = remoteTime_.integerTime () - remoteTime_.tickInterval ();
    double c = ((double) (localTime_->integerTime () - prevSampleTime)
		/ (double) remoteTime_.tickInterval ());

    MUSIC_LOGR ("interpolationCoefficient = " << c);
    // NOTE: preliminary implementation which just provides
    // the functionality specified in the API
    if (interpolate_)
      return c;
    else
      return round (c);
  }

  void
  InterpolatingContInputConnector::postCommunication ()
  {
	  if (first_)
	  {
		  collector_.collect (sampler_.insert ());
		  remoteTick ();
		  first_ = false;
	  }
	  if (sample ())
	  {
		  collector_.collect (sampler_.insert ());
		  remoteTick ();
	  }
	  sampler_.interpolateToApplication (interpolationCoefficient ());
  }


  /********************************************************************
   *
   * Event Connectors
   *
   ********************************************************************/


  Subconnector*
  EventOutputConnector::makeSubconnector (int remoteRank)
  {
    return new EventOutputSubconnector (//&synch,
					intercomm,
					remoteLeader (),
					remoteRank,
					receiverPortCode ());
  }

  void
  EventOutputConnector::addRoutingInterval (IndexInterval i,
					    Subconnector* subconn)
  {
	OutputSubconnector*osubconn= dynamic_cast<OutputSubconnector*>(subconn);
    routingMap_-> insert (i, osubconn->buffer ());
  }
  

  Subconnector*
  EventInputConnector::makeSubconnector (int remoteRank)
  {
	int receiverRank = intercomm.Get_rank ();
    if (type_ == Index::GLOBAL)
      return new EventInputSubconnectorGlobal (//&synch,
					       intercomm,
					       remoteLeader (),
					       remoteRank,
					       receiverRank,
					       receiverPortCode (),
					       handleEvent_.global ());
    else
      return new EventInputSubconnectorLocal (//&synch,
					      intercomm,
					      remoteLeader (),
					      remoteRank,
					      receiverRank,
					      receiverPortCode (),
					      handleEvent_.local ());
  }



 void EventInputConnector::addRoutingInterval(IndexInterval i, Subconnector* subconn)
 {
	 //routingMap_-> insert (i, handleEvent_.global());
 }


  /********************************************************************
   *
   * Message Connectors
   *
   ********************************************************************/
  
  MessageOutputConnector::MessageOutputConnector (ConnectorInfo connInfo,
						  SpatialOutputNegotiator*
						  spatialNegotiator,
						  MPI::Intracomm comm,
						  std::vector<FIBO*>& buffers)
    : Connector (connInfo, spatialNegotiator, comm),
      buffer (1),
      bufferAdded (false),
      buffers_ (buffers)
  {
  }



  
  Subconnector*
  MessageOutputConnector::makeSubconnector (int remoteRank)
  {
    return new MessageOutputSubconnector (//&synch,
					  intercomm,
					  remoteLeader (),
					  remoteRank,
					  receiverPortCode (),
					  &buffer);
  }


  void
  MessageOutputConnector::addRoutingInterval (IndexInterval i,
					      Subconnector* subconn)
  {
    if (!bufferAdded)
      {
	buffers_.push_back (&buffer);
	bufferAdded = true;
      }
  }
  
  


  
  void
  MessageOutputConnector::postCommunication ()
  {
 //   if (synch.communicate ())
      buffer.clear ();
  }

  
  MessageInputConnector::MessageInputConnector (ConnectorInfo connInfo,
						SpatialInputNegotiator* spatialNegotiator,
						MessageHandler* handleMessage,
						Index::Type type,
						MPI::Intracomm comm)
    : Connector (connInfo, spatialNegotiator, comm),
      handleMessage_ (handleMessage),
      type_ (type)
  {
  }



  
  Subconnector*
  MessageInputConnector::makeSubconnector (int remoteRank)
  {
	  int receiverRank = intercomm.Get_rank ();
    return new MessageInputSubconnector (//&synch,
					 intercomm,
					 remoteLeader (),
					 remoteRank,
					 receiverRank,
					 receiverPortCode (),
					 handleMessage_);
  }

}
#endif

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
#include "music/debug.hh"

// connector.hh needs to be included first since it causes inclusion
// of mpi.h (in data_map.hh).  mpi.h must be included before other
// header files on BG/L
#include "music/connector.hh"

#include "music/error.hh"
#include "music/communication.hh"

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


  void
  OutputConnector::spatialNegotiation
  (std::vector<OutputSubconnector*>& osubconn,
   std::vector<InputSubconnector*>&)
  {
    std::map<int, OutputSubconnector*> subconnectors;
    for (NegotiationIterator i
	   = spatialNegotiator_->negotiate (comm,
					    intercomm,
					    info.nProcesses (),
					    this); // only for debugging
	 !i.end ();
	 ++i)
      {
	std::map<int, OutputSubconnector*>::iterator c
	  = subconnectors.find (i->rank ());
	OutputSubconnector* subconn;
	if (c != subconnectors.end ())
	  subconn = c->second;
	else
	  {
	    subconn = makeOutputSubconnector (i->rank ());
	    subconnectors.insert (std::make_pair (i->rank (), subconn));
	    osubconn.push_back (subconn);
	  }
	MUSIC_LOG (MPI::COMM_WORLD.Get_rank ()
		   << ": ("
		   << i->begin () << ", "
		   << i->end () << ", "
		   << i->local () << ") -> " << i->rank ());
	addRoutingInterval (i->interval (), subconn);
      }
  }


  void
  OutputConnector::addRoutingInterval (IndexInterval, OutputSubconnector*)
  {
    // Default: Do nothing
  }



  // NOTE: code repetition (OutputConnector::spatialNegotiation)
  void
  InputConnector::spatialNegotiation
  (std::vector<OutputSubconnector*>&,
   std::vector<InputSubconnector*>& isubconn)
  {
    std::map<int, InputSubconnector*> subconnectors;
    int receiverRank = intercomm.Get_rank ();
    for (NegotiationIterator i
	   = spatialNegotiator_->negotiate (comm,
					    intercomm,
					    info.nProcesses (),
					    this); // only for debugging
	 !i.end ();
	 ++i)
      {
	std::map<int, InputSubconnector*>::iterator c
	  = subconnectors.find (i->rank ());
	InputSubconnector* subconn;
	if (c != subconnectors.end ())
	  subconn = c->second;
	else
	  {
	    subconn = makeInputSubconnector (i->rank (), receiverRank);
	    subconnectors.insert (std::make_pair (i->rank (), subconn));
	    isubconn.push_back (subconn);
	  }
	MUSIC_LOG (MPI::COMM_WORLD.Get_rank ()
		   << ": " << i->rank () << " -> ("
		   << i->begin () << ", "
		   << i->end () << ", "
		   << i->local () << ")");
	addRoutingInterval (i->interval (), subconn);
      }
  }


  void
  InputConnector::addRoutingInterval (IndexInterval, InputSubconnector*)
  {
    // Default: Do nothing
  }


  /********************************************************************
   *
   * Cont Connectors
   *
   ********************************************************************/

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

  
  OutputSubconnector*
  ContOutputConnector::makeOutputSubconnector (int remoteRank)
  {
    return new ContOutputSubconnector (synchronizer (),
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

    if (tickInterval < remoteTickInterval (tickInterval))
      connector = new InterpolatingContOutputConnector (*this);
    else
      connector = new PlainContOutputConnector (*this);
    
    delete this; // delete ourselves!
    return connector;
  }

  
  void
  ContOutputConnector::addRoutingInterval (IndexInterval i,
					   OutputSubconnector* osubconn)
  {
    distributor_.addRoutingInterval (i, osubconn->buffer ());
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
    synch.initialize ();

    // put one element in send buffers
    distributor_.distribute ();
  }


  void
  PlainContOutputConnector::tick (bool& requestCommunication)
  {
    if (synch.sample ())
      {
	// copy application data to send buffers
	distributor_.distribute ();
      }

    synch.tick ();
    if (synch.communicate ())
      {
	requestCommunication = true;
      }
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
    distributor_.configure (sampler_.interpolationDataMap ());
    distributor_.initialize ();
    synch.initialize ();

    // put one element in send buffers
    sampler_.sample ();
    sampler_.interpolate (1.0);
    distributor_.distribute ();
  }

  
  void
  InterpolatingContOutputConnector::tick (bool& requestCommunication)
  {
    synch.tick ();
    if (synch.sample ())
      // sampling before and after time of receiver tick
      sampler_.sampleOnce ();
    if (synch.interpolate ())
      {
	sampler_.interpolate (synch.interpolationCoefficient ());
	synch.remoteTick ();
	distributor_.distribute ();
      }
    if (synch.communicate ())
      requestCommunication = true;
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

  
  InputSubconnector*
  ContInputConnector::makeInputSubconnector (int remoteRank, int receiverRank)
  {
    return new ContInputSubconnector (synchronizer (),
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

    if (tickInterval < rTickInterval
	|| (tickInterval == rTickInterval
	    && !divisibleDelay (localTime)))
      connector = new InterpolatingContInputConnector (*this);
    else
      connector = new PlainContInputConnector (*this);
    
    return connector;
  }

  
  void
  ContInputConnector::addRoutingInterval (IndexInterval i,
					  InputSubconnector* isubconn)
  {
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
    collector_.configure (sampler_.dataMap (), synch.allowedBuffered () + 1);
    collector_.initialize ();
    synch.initialize ();
  }
  

  void
  PlainContInputConnector::tick (bool& requestCommunication)
  {
    synch.tick ();
    if (synch.communicate ())
      requestCommunication = true;
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
    collector_.configure (sampler_.interpolationDataMap (),
			  synch.allowedBuffered () + 1);
    collector_.initialize ();
    synch.initialize ();
  }


  void
  InterpolatingContInputConnector::tick (bool& requestCommunication)
  {
    synch.tick ();
    if (synch.communicate ())
      requestCommunication = true;
  }


  void
  InterpolatingContInputConnector::postCommunication ()
  {
    if (first_)
      {
	collector_.collect (sampler_.insert ());
	synch.remoteTick ();
	first_ = false;
      }
    if (synch.sample ())
      {
	collector_.collect (sampler_.insert ());
	synch.remoteTick ();
      }
    sampler_.interpolateToApplication (synch.interpolationCoefficient ());
  }


  /********************************************************************
   *
   * Event Connectors
   *
   ********************************************************************/
  
  EventOutputConnector::EventOutputConnector (ConnectorInfo connInfo,
					      SpatialOutputNegotiator* spatialNegotiator,
					      MPI::Intracomm comm,
					      EventRoutingMap* routingMap)
    : Connector (connInfo, spatialNegotiator, comm),
      routingMap_ (routingMap)
  {
  }

  
  void
  EventOutputConnector::initialize ()
  {
    synch.initialize ();
  }

  
  OutputSubconnector*
  EventOutputConnector::makeOutputSubconnector (int remoteRank)
  {
    return new EventOutputSubconnector (&synch,
					intercomm,
					remoteLeader (),
					remoteRank,
					receiverPortCode ());
  }


  void
  EventOutputConnector::addRoutingInterval (IndexInterval i,
					    OutputSubconnector* osubconn)
  {
    routingMap_->insert (i, osubconn->buffer ());
  }
  
  
  void
  EventOutputConnector::tick (bool& requestCommunication)
  {
    synch.tick ();
    // Only assign requestCommunication if true
    if (synch.communicate ())
      requestCommunication = true;
  }

  
  EventInputConnector::EventInputConnector (ConnectorInfo connInfo,
					    SpatialInputNegotiator* spatialNegotiator,
					    EventHandlerPtr handleEvent,
					    Index::Type type,
					    MPI::Intracomm comm)
    : Connector (connInfo, spatialNegotiator, comm),
      handleEvent_ (handleEvent),
      type_ (type)
  {
  }

  
  void
  EventInputConnector::initialize ()
  {
    synch.initialize ();
  }

  
  InputSubconnector*
  EventInputConnector::makeInputSubconnector (int remoteRank, int receiverRank)
  {
    if (type_ == Index::GLOBAL)
      return new EventInputSubconnectorGlobal (&synch,
					       intercomm,
					       remoteLeader (),
					       remoteRank,
					       receiverRank,
					       receiverPortCode (),
					       handleEvent_.global ());
    else
      return new EventInputSubconnectorLocal (&synch,
					      intercomm,
					      remoteLeader (),
					      remoteRank,
					      receiverRank,
					      receiverPortCode (),
					      handleEvent_.local ());
  }


  void
  EventInputConnector::tick (bool& requestCommunication)
  {
    synch.tick ();
    // Only assign requestCommunication if true
    if (synch.communicate ())
      requestCommunication = true;
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

  
  void
  MessageOutputConnector::initialize ()
  {
    synch.initialize ();
  }

  
  OutputSubconnector*
  MessageOutputConnector::makeOutputSubconnector (int remoteRank)
  {
    return new MessageOutputSubconnector (&synch,
					  intercomm,
					  remoteLeader (),
					  remoteRank,
					  receiverPortCode (),
					  &buffer);
  }


  void
  MessageOutputConnector::addRoutingInterval (IndexInterval,
					      OutputSubconnector*)
  {
    if (!bufferAdded)
      {
	buffers_.push_back (&buffer);
	bufferAdded = true;
      }
  }
  
  
  void
  MessageOutputConnector::tick (bool& requestCommunication)
  {
    synch.tick ();
    // Only assign requestCommunication if true
    if (synch.communicate ())
      requestCommunication = true;
  }

  
  void
  MessageOutputConnector::postCommunication ()
  {
    if (synch.communicate ())
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


  void
  MessageInputConnector::initialize ()
  {
    synch.initialize ();
  }

  
  InputSubconnector*
  MessageInputConnector::makeInputSubconnector (int remoteRank, int receiverRank)
  {
    return new MessageInputSubconnector (&synch,
					 intercomm,
					 remoteLeader (),
					 remoteRank,
					 receiverRank,
					 receiverPortCode (),
					 handleMessage_);
  }


  void
  MessageInputConnector::tick (bool& requestCommunication)
  {
    synch.tick ();
    // Only assign requestCommunication if true
    if (synch.communicate ())
      requestCommunication = true;
  }

}

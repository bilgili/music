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

#include "music/connector.hh"
#include "music/debug.hh"

namespace MUSIC {

  Connector::Connector (ConnectorInfo info_,
			SpatialNegotiator* spatialNegotiator,
			MPI::Intracomm c)
    : info (info_),
      spatialNegotiator_ (spatialNegotiator),
      comm (c)
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
    MUSIC_LOG (comm.Get_rank () << ": " << comm);
    MUSIC_LOG (comm.Get_rank () << ": remote = " << info.remoteLeader ());
    intercomm = comm.Create_intercomm (0,
				       MPI::COMM_WORLD, //*fixme* recursive?
				       info.remoteLeader (),
				       0); //*fixme* tag
  }


  void
  OutputConnector::spatialNegotiation
  (std::vector<OutputSubconnector*>& osubconn,
   std::vector<InputSubconnector*>& isubconn)
  {
    std::map<int, OutputSubconnector*> subconnectors;
    for (NegotiationIterator i
	   = spatialNegotiator_->negotiate (comm,
					    intercomm,
					    info.nProcesses ());
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
  InputConnector::spatialNegotiation
  (std::vector<OutputSubconnector*>& osubconn,
   std::vector<InputSubconnector*>& isubconn)
  {
    std::map<int, InputSubconnector*> subconnectors;
    int receiverRank = intercomm.Get_rank ();
    for (NegotiationIterator i = spatialNegotiator_->negotiate (comm,
								intercomm,
								info.nProcesses ());
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
      }
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
				    0, 0, 0, 0); /*fixme* tags*/
      }
    // broadcast to peers
    comm.Bcast (&sRemoteTickInterval, 2, MPI::UNSIGNED_LONG, 0);
    return sRemoteTickInterval.deserialize ();
  }

  ContOutputConnector::ContOutputConnector (ConnectorInfo connInfo,
					    SpatialOutputNegotiator* spatialNegotiator,
					    MPI::Intracomm comm,
					    Sampler& sampler)
    : Connector (connInfo, spatialNegotiator, comm),
      ContConnector (sampler)
  {
  }

  
  OutputSubconnector*
  ContOutputConnector::makeOutputSubconnector (int remoteRank)
  {
    return new ContOutputSubconnector (synchronizer (),
				       intercomm,
				       remoteRank,
				       receiverPortName ());
  }
  

  Connector*
  ContOutputConnector::specialize (ClockState tickInterval)
  {
    Connector* connector;
    if (tickInterval < remoteTickInterval (tickInterval))
      connector = new InterpolatingContOutputConnector (*this);
    else
      connector = new PlainContOutputConnector (*this);
    *ref_ = connector;
    delete this; // delete ourselves!!
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
    synch.initialize ();
  }


  void
  PlainContOutputConnector::tick (bool& requestCommunication)
  {
    distributor_.distribute ();
    synch.tick ();
    if (synch.communicate ())
      requestCommunication = true;
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
    synch.initialize ();
  }

  
  void
  InterpolatingContOutputConnector::tick (bool& requestCommunication)
  {
    if (synch.sample ())
      // sampling before and after time of receiver tick
      sampler_.sample ();
    if (synch.interpolate ())
      {
	sampler_.interpolate (synch.interpolationCoefficient ());
	distributor_.distribute ();
      }
    synch.tick ();
    if (synch.communicate ())
      requestCommunication = true;
  }


  ContInputConnector::ContInputConnector (ConnectorInfo connInfo,
					  SpatialInputNegotiator* spatialNegotiator,
					  MPI::Intracomm comm,
					  Sampler& sampler)
    : Connector (connInfo, spatialNegotiator, comm),
      ContConnector (sampler)
  {
  }

  
  InputSubconnector*
  ContInputConnector::makeInputSubconnector (int remoteRank, int receiverRank)
  {
    return new ContInputSubconnector (synchronizer (),
				      intercomm,
				      remoteRank,
				      receiverRank,
				      receiverPortName ());
  }
  

  Connector*
  ContInputConnector::specialize (ClockState tickInterval)
  {
    Connector* connector;
    if (tickInterval < remoteTickInterval (tickInterval))
      connector = new InterpolatingContInputConnector (*this);
    else
      connector = new PlainContInputConnector (*this);
    *ref_ = connector;
    delete this; // delete ourselves!!
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
    collector_.configure (sampler_.dataMap (), synch.allowedBuffered ());
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


  void
  InterpolatingContInputConnector::initialize ()
  {
    collector_.configure (sampler_.interpolationDataMap (),
			  synch.allowedBuffered ());
    synch.initialize ();
  }
  

  InterpolatingContInputConnector::InterpolatingContInputConnector
  (ContInputConnector& connector)
    : Connector (connector),
      ContInputConnector (connector)
  {
  }


  void
  InterpolatingContInputConnector::tick (bool& requestCommunication)
  {
    sample = synch.sample ();
    interpolationCoefficient = synch.interpolationCoefficient ();
    synch.tick ();
    if (synch.communicate ())
      requestCommunication = true;
  }


  void
  InterpolatingContInputConnector::postCommunication ()
  {
    if (sample)
      collector_.collect (sampler_.insert ());
    sampler_.interpolateToApplication (interpolationCoefficient);
  }


  /********************************************************************
   *
   * Event Connectors
   *
   ********************************************************************/
  
  EventOutputConnector::EventOutputConnector (ConnectorInfo connInfo,
					      SpatialOutputNegotiator* spatialNegotiator,
					      MPI::Intracomm comm,
					      EventRouter& router)
    : Connector (connInfo, spatialNegotiator, comm),
      router_ (router)
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
					remoteRank,
					receiverPortName ());
  }


  void
  EventOutputConnector::addRoutingInterval (IndexInterval i,
					    OutputSubconnector* osubconn)
  {
    router_.insertRoutingInterval (i, osubconn->buffer ());
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
					       remoteRank,
					       receiverRank,
					       receiverPortName (),
					       handleEvent_.global ());
    else
      return new EventInputSubconnectorLocal (&synch,
					      intercomm,
					      remoteRank,
					      receiverRank,
					      receiverPortName (),
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
						  EventRouter& router)
    : Connector (connInfo, spatialNegotiator, comm),
      router_ (router)
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
					remoteRank,
					receiverPortName ());
  }


  void
  MessageOutputConnector::addRoutingInterval (IndexInterval i,
					    OutputSubconnector* osubconn)
  {
    router_.insertRoutingInterval (i, osubconn->buffer ());
  }
  
  
  void
  MessageOutputConnector::tick (bool& requestCommunication)
  {
    synch.tick ();
    // Only assign requestCommunication if true
    if (synch.communicate ())
      requestCommunication = true;
  }

  
  MessageInputConnector::MessageInputConnector (ConnectorInfo connInfo,
					    SpatialInputNegotiator* spatialNegotiator,
					    MessageHandlerPtr handleMessage,
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
    return new MessageInputSubconnectorGlobal (&synch,
					       intercomm,
					       remoteRank,
					       receiverRank,
					       receiverPortName (),
					       handleMessage_.global ());
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

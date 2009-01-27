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

  Connector::Connector (ConnectorInfo _info,
			SpatialNegotiator* _spatialNegotiator,
			MPI::Intracomm c)
    : info (_info),
      spatialNegotiator (_spatialNegotiator),
      comm (c)
  {
  }

  
  MPI::Intercomm
  Connector::createIntercomm ()
  {
    return comm.Create_intercomm (0,
				  MPI::COMM_WORLD, //*fixme* recursive?
				  info.remoteLeader (),
				  0); //*fixme* tag
  }


  void
  OutputConnector::tick (bool& requestCommunication)
  {
    synch.tick ();
    // Only assign requestCommunication if true
    if (synch.communicate ())
      requestCommunication = true;
  }

  
  void
  InputConnector::tick (bool& requestCommunication)
  {
    synch.tick ();
    // Only assign requestCommunication if true
    if (synch.communicate ())
      requestCommunication = true;
  }

  
  void
  ContConnector::swapBuffers (contDataT*& b1, contDataT*& b2)
  {
    contDataT* tmp;
    tmp = b1;
    b1 = b2;
    b2 = tmp;
  }
  

  void
  ContInputConnector::receive ()
  {
  }

  
  void
  FastContOutputConnector::interpolateTo (int start, int end, contDataT* data)
  {
  }
  
  
  void
  FastContOutputConnector::interpolateToBuffers ()
  {
#if 0
    std::vector<OutputSubconnector*>::iterator i = subconnectors.begin ();
    for (; i != subconnectors.end (); ++i)
      {
	contDataT* data = (contDataT*) (*i)->buffer.insert ();
	interpolateTo ((*i)->startIdx (), (*i)->endIdx (), data);
      }
#endif
  }

  
  void
  FastContOutputConnector::mark ()
  {
#if 0
    std::vector<OutputSubconnector*>::iterator i = subconnectors.begin ();
    for (; i != subconnectors.end (); ++i)
      {
	ContOutputSubconnector* subcon = (ContOutputSubconnector*) *i;
	subcon->buffer.mark ();
      }
#endif
  }

  
  void
  ContOutputConnector::send ()
  {
#if 0
    std::vector<OutputSubconnector*>::iterator i = subconnectors.begin ();
    for (; i != subconnectors.end (); ++i)
      {
	ContOutputSubconnector* subcon = (ContOutputSubconnector*) *i;
	subcon->send ();
      }
#endif
  }


  void
  FastContOutputConnector::applicationTo (contDataT* data)
  {
  }
  
  
  void
  SlowContInputConnector::toApplication ()
  {
  }

  
  void
  FastContOutputConnector::tick ()
  {
    if (synch.sample ())
      {
	swapBuffers (prevSample, sample);
	// data is copied into sections destined to different subconnectors
	applicationTo (sample);
	interpolateToBuffers ();
      }
    if (synch.mark ())
      mark ();
    if (synch.communicate ())
      send ();
  }


  void
  SlowContInputConnector::buffersToApplication ()
  {
  }

  
  void
  SlowContInputConnector::tick ()
  {
    if (synch.communicate ())
      receive ();
    buffersToApplication ();
  }


  void
  SlowContOutputConnector::applicationToBuffers ()
  {
  }

  
  void
  SlowContOutputConnector::tick ()
  {
    applicationToBuffers ();
    if (synch.communicate ())
      send ();
  }


  void
  FastContInputConnector::buffersTo (contDataT* data)
  {
  }

  
  void
  FastContInputConnector::interpolateToApplication ()
  {
  }

  
  void
  FastContInputConnector::tick ()
  {
    if (synch.communicate ())
      receive ();
    swapBuffers (prevSample, sample);
    buffersTo (sample);
    interpolateToApplication ();
  }


  EventConnector::EventConnector (ConnectorInfo _info,
				    SpatialNegotiator* _spatialNegotiator,
				    MPI::Intracomm c)
    : Connector (_info, _spatialNegotiator, c)
  {
  }
  

  EventOutputConnector::EventOutputConnector (ConnectorInfo connInfo,
					      SpatialOutputNegotiator* _spatialNegotiator,
					      MPI::Intracomm comm,
					      EventRouter& _router)
    : Connector (connInfo, _spatialNegotiator, comm),
      EventConnector (connInfo, _spatialNegotiator, comm),
      router (_router)
  {
  }

  
  void
  EventOutputConnector::spatialNegotiation
  (std::vector<OutputSubconnector*>& osubconn,
   std::vector<InputSubconnector*>& isubconn)
  {
    std::map<int, EventOutputSubconnector*> subconnectors;
    MPI::Intercomm intercomm = createIntercomm ();
    for (NegotiationIterator i
	   = spatialNegotiator->negotiate (comm,
					   intercomm,
					   info.nProcesses ());
	 !i.end ();
	 ++i)
      {
	std::map<int, EventOutputSubconnector*>::iterator c
	  = subconnectors.find (i->rank ());
	EventOutputSubconnector* subconn;
	if (c != subconnectors.end ())
	  subconn = c->second;
	else
	  {
	    subconn = new EventOutputSubconnector (&synch,
						     intercomm,
						     i->rank (),
						     receiverPortName ());
	    subconnectors.insert (std::make_pair (i->rank (), subconn));
	    osubconn.push_back (subconn);
	  }
	MUSIC_LOG (MPI::COMM_WORLD.Get_rank ()
		   << ": ("
		   << i->begin () << ", "
		   << i->end () << ", "
		   << i->local () << ") -> " << i->rank ());
	router.insertRoutingInterval (i->interval (), subconn->buffer ());
      }
  }

  
  EventInputConnector::EventInputConnector (ConnectorInfo connInfo,
					    SpatialInputNegotiator* spatialNegotiator,
					    EventHandlerPtr _handleEvent,
					    Index::Type _type,
					    MPI::Intracomm comm)
    : Connector (connInfo, spatialNegotiator, comm),
      EventConnector (connInfo, spatialNegotiator, comm),
      handleEvent (_handleEvent),
      type (_type)
  {
  }

  
  void
  EventInputConnector::spatialNegotiation
  (std::vector<OutputSubconnector*>& osubconn,
   std::vector<InputSubconnector*>& isubconn)
  {
    std::map<int, EventInputSubconnector*> subconnectors;
    MPI::Intercomm intercomm = createIntercomm ();
    int receiverRank = intercomm.Get_rank ();
    for (NegotiationIterator i = spatialNegotiator->negotiate (comm,
							 intercomm,
							 info.nProcesses ());
	 !i.end ();
	 ++i)
      {
	std::map<int, EventInputSubconnector*>::iterator c
	  = subconnectors.find (i->rank ());
	EventInputSubconnector* subconn;
	if (c != subconnectors.end ())
	  subconn = c->second;
	else
	  {
	    if (type == Index::GLOBAL)
	      subconn
		= new EventInputSubconnectorGlobal (&synch,
						       intercomm,
						       i->rank (),
						       receiverRank,
						       receiverPortName (),
						       handleEvent.global ());
	    else
	      subconn
		= new EventInputSubconnectorLocal (&synch,
						      intercomm,
						      i->rank (),
						      receiverRank,
						      receiverPortName (),
						      handleEvent.local ());
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

}

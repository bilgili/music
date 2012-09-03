/*
 *  This file is part of MUSIC.
 *  Copyright (C) 2012 INCF
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

//#define MUSIC_DEBUG
#include "music/scheduler.hh"

#include "music/debug.hh"

#if MUSIC_USE_MPI

#include <cmath>
#include <iostream>

#ifdef MUSIC_DEBUG
#include <cstdlib>
#endif

namespace MUSIC {

  Scheduler::Node::Node (int id, const Clock &localTime, int leader, int nProcs)
    : id_ (id), localTime_ (localTime), leader_ (leader), nProcs_ (nProcs)
  {
  }

  void
  Scheduler::Node::advance ()
  {
#ifdef MUSIC_DEBUG
    double time = localTime_.time ();
#endif
    localTime_.tick ();
    MUSIC_LOG0 ("advanced " << id_ <<" from "<<time << " to " <<  localTime_.time ()) ;
  }

  void
  Scheduler::Node::addConnection (Connection *conn, bool input)
  {
    if (input)
      inputConnections_.push_back (conn);
    else
      outputConnections_.push_back (conn);
  }

  double
  Scheduler::Node::nextReceive () const
  {
    std::vector<Connection*>::const_iterator conn;
    double nextTime = std::numeric_limits<double>::infinity ();
    for (conn = inputConnections_.begin ();
	 conn < inputConnections_.end ();
	 conn++)
      {
	if ((*conn)->nextReceive ().time () < nextTime)
	  nextTime = (*conn)->nextReceive ().time ();
      }
    return nextTime;
  }

  Scheduler::Connection::Connection (int pre,
				     int post,
				     const ClockState &latency,
				     int maxBuffered,
				     bool interpolate,
				     int port_code)
    : pre_id (pre),
      post_id (post),
      latency_ (latency),
      maxBuffered_ (maxBuffered),
      interpolate_ (interpolate),
      port_code_ (port_code)
  {
  }
  
  void
  Scheduler::Connection::initialize (std::vector<Node*> &nodes)
  {
    pre_ = nodes.at (pre_id);
    post_ = nodes.at (post_id);
    pre_->addConnection (this);
    post_->addConnection (this, true);
    nextSend_.configure (pre_->localTime ().timebase (),
			 pre_->localTime ().tickInterval ());
    nextReceive_.configure (post_->localTime ().timebase (),
			    post_->localTime ().tickInterval ());
    advance ();
  }

  void
  Scheduler::Connection::advance ()
  {
    _advance ();
    Clock  r = nextReceive_;
    Clock  s = nextSend_;
    _advance ();
    while (nextReceive_ == r)
      {
	s = nextSend_;
	_advance ();
      }
    nextReceive_ = r;
    nextSend_ = s;
    MUSIC_LOG0 (pre_id << "->"<<post_id << "::"<<nextSend_.time () << "::" <<  nextReceive_.time ()) ;
  }

  void
  Scheduler::Connection::_advance ()
  {
    ClockState limit = (nextSend_.integerTime ()
			+ latency_
			- nextReceive_.tickInterval ());
    while (nextReceive_.integerTime () <= limit)
      nextReceive_.tick ();

    /*limit = nextReceive_.integerTime () + nextReceive_.tickInterval () - latency_;
      int bCount = 0;
      while (nextSend_.integerTime () < limit) {
      nextSend_.tick ();
      ++bCount;
      }
      // Advance send time according to precalculated buffer
      if (bCount < maxBuffered_)
      nextSend_.ticks (maxBuffered_ - bCount);
    */
    nextSend_.ticks (maxBuffered_+1);
  }
  
  Scheduler::Scheduler (int node_id)
    :self_node (node_id)
  {
  }

  Scheduler::~Scheduler ()
  {
    for (std::vector<Connection*>::iterator conn=connections.begin ();
	 conn < connections.end ();
	 conn++)
      delete (*conn);
    for (std::vector<Node*>::iterator node=nodes.begin ();
	 node < nodes.end ();
	 node++)
      delete *node;
  }

  void
  Scheduler::addNode (int id, const Clock &localTime, int leader, int nProcs)
  {
    nodes.push_back (new Node (id, localTime, leader, nProcs));
  }

  void
  Scheduler::addConnection (int pre_id,
			    int post_id,
			    const ClockState &latency,
			    int maxBuffered,
			    bool interpolate,
			    int port_code)
  {
    connections.push_back (new Connection (pre_id,
					   post_id,
					   latency,
					   maxBuffered,
					   interpolate,
					   port_code));
  }

  void
  Scheduler::initialize (std::vector<Connector*>& connectors)
  {
    std::vector<Connection*>::iterator conn;

    //MUSIC_LOGR ("#of nodes:" << nodes.size () << ":#of connections:" <<  connections.size ());
    for (conn = connections.begin (); conn < connections.end (); conn++)
      {
	(*conn)->initialize (nodes);
	bool foundLocalConnector = false;
	for (std::vector<Connector*>::iterator c = connectors.begin ();
	     c != connectors.end ();
	     ++c)
	  {
	    if ((*conn)->portCode () == (*c)->receiverPortCode ())
	      {
		foundLocalConnector = true;
		(*conn)->setConnector ((*c));
		(*c)->setInterpolate ((*conn)->getInterpolate ());
		(*c)->setLatency ((*conn)->getLatency ());
		(*c)->initialize ();
	      }
	  }
	if (!foundLocalConnector)
	  (*conn)->setConnector
	    (new ProxyConnector ((*conn)->preNode ()->leader (),
				 (*conn)->preNode ()->nProcs (),
				 (*conn)->postNode ()->leader (),
				 (*conn)->postNode ()->nProcs ()));
      }
  }

  void
  Scheduler::nextCommunication (std::vector<std::pair<double, Connector *> > &schedule)
  {
    while (schedule.empty ())
      {
	std::vector<Node*>::iterator node;
	for ( node=nodes.begin (); node < nodes.end (); node++ )
	  {
	    if ((*node)->nextReceive () > (*node)->localTime ().time ())
	      (*node)->advance ();

	    std::vector<Connection*>* conns = (*node)->outputConnections ();
	    std::vector<Connection*>::iterator conn;
	    for (conn = conns->begin (); conn < conns->end (); conn++)
	      //do we have data ready to be sent?
	      if ((*conn)->nextSend () <= (*conn)->preNode ()->localTime ()
		  && (*conn)->nextReceive () == (*conn)->postNode ()->localTime ())
		{
		  if (self_node == (*conn)->postNode ()->getId () //input
		      || self_node == (*conn)->preNode ()->getId ()) //output
		    {
		      double nextComm
			= (self_node == (*conn)->postNode ()->getId ()
			   ? (*conn)->postNode ()->localTime ().time ()
			   : (*conn)->preNode ()->localTime ().time ());
		      schedule.push_back (std::pair<double, Connector*>(nextComm, (*conn)->getConnector ()));
		    }

		  MUSIC_LOG0 ("Scheduled communication:"<< (*conn)->preNode ()->getId () <<"->"<< (*conn)->postNode ()->getId () << "at(" << (*conn)->preNode ()->localTime ().time () << ", "<< (*conn)->postNode ()->localTime ().time () <<")");
		  (*conn)->advance ();
		}
	  }
      }
  }
}

#endif

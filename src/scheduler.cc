/*
 *  This file is part of MUSIC.
 *  Copyright (C) 2008, 2009, 2010 INCF
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
#include <mpi.h>
#include "music/debug.hh" // Must be included first on BG/L

#include <cmath>

#include "music/scheduler.hh"
#include <iostream>
#ifdef MUSIC_DEBUG
#include <cstdlib>
#endif

namespace MUSIC {
Scheduler::Node::Node(int id, const Clock &localTime)
:id_(id),
 localTime_(localTime){

}
void Scheduler::Node::advance(){
	double time = localTime_.time();
	localTime_.tick();
	MUSIC_LOG0("advanced " << id_ <<" from "<<time << " to " <<  localTime_.time()) ;

}
void Scheduler::Node::addConnection(Connection *conn, bool input){
	if (input)
		inputConnections_.push_back(conn);
	else
		outputconnections_.push_back(conn);

}

double Scheduler::Node::nextReceive() const{
	std::vector<Connection*>::const_iterator conn;
	double nextTime = std::numeric_limits<double>::infinity();
	for ( conn = inputConnections_.begin(); conn < inputConnections_.end(); conn++){
		if ((*conn)->nextReceive().time() < nextTime)
			nextTime = (*conn)->nextReceive().time();
	}
	return nextTime;
}

Scheduler::Connection::Connection(int pre,int post,const ClockState &latency,int maxBuffered, int port_code)
:pre_id(pre),
 post_id(post),
 latency_(latency),
 maxBuffered_(maxBuffered),
 port_code_(port_code){
}
void Scheduler::Connection::initialize(std::vector<Node*> &nodes){
	pre_ = nodes.at(pre_id);
	post_ = nodes.at(post_id);
	pre_->addConnection(this);
	post_->addConnection(this,true);
	nextSend_.configure(pre_->localTime().timebase(), pre_->localTime().tickInterval());
	nextReceive_.configure(post_->localTime().timebase(), post_->localTime().tickInterval());
	advance();
}
void Scheduler::Connection::advance(){
	_advance();
	Clock  r = nextReceive_;
	Clock  s = nextSend_;
	_advance();
	while (nextReceive_ == r){
		s = nextSend_;
		_advance();
	}
	nextReceive_ = r;
	nextSend_ = s;
}
void Scheduler::Connection::_advance(){
	ClockState limit = nextSend_.integerTime () + latency_ - nextReceive_.tickInterval ();
	while (nextReceive_.integerTime () <= limit)	nextReceive_.tick ();
	limit = nextReceive_.integerTime () + nextReceive_.tickInterval () - latency_;
	int bCount = 0;
	while (nextSend_.integerTime () <= limit) {
		nextSend_.tick ();
		++bCount;
	}
	// Advance send time according to precalculated buffer
	if (bCount < maxBuffered_)
		nextSend_.ticks (maxBuffered_ - bCount);
}
Scheduler::Scheduler(int node_id)
:self_node(node_id){

}

Scheduler::~Scheduler(){
	for (std::vector<Connection*>::iterator conn=connections.begin(); conn < connections.end(); conn++ )
		delete (*conn);
	for (std::vector<Node*>::iterator node=nodes.begin(); node < nodes.end(); node++ )
		delete (*node);
}
void Scheduler::addNode(int id, const Clock &localTime){
	nodes.push_back(new Node(id, localTime));
}
void Scheduler::addConnection(int pre_id,int post_id,const ClockState &latency,int maxBuffered, int port_code){
	connections.push_back(new Connection(pre_id,post_id,latency,maxBuffered, port_code));
}
void
Scheduler::initialize(std::vector<Connector*> &connectors){

	std::vector<Connection*>::iterator conn;

	MUSIC_LOGR ("#of nodes:" << nodes.size() << ":#of connections:" <<  connections.size());
	for ( conn = connections.begin(); conn < connections.end(); conn++){
		(*conn)->initialize(nodes);
	}
	for (std::vector<Connector*>::iterator c = connectors.begin (); c != connectors.end (); ++c){
		for ( conn = connections.begin(); conn < connections.end(); conn++){
			if((*conn)->portCode() == (*c)->receiverPortCode())
				(*conn)->setConnector((*c));
		}
	}

}
void
Scheduler::nextCommunication (Clock &nextComm, std::queue<Connector *> &connectors)
{
	bool scheduled = false;
	while (!scheduled){
		std::vector<Node*>::iterator node;
		for ( node=nodes.begin(); node < nodes.end(); node++ ){

			if ((*node)->nextReceive() > (*node)->localTime().time())
				(*node)->advance();

			std::vector<Connection*> conns = (*node)->outputConnections();
			std::vector<Connection*>::iterator conn;
			for ( conn = conns.begin(); conn < conns.end(); conn++){
				if ((*conn)->nextSend() <= (*conn)->preNode()->localTime()
						&& (*conn)->nextReceive() == (*conn)->postNode()->localTime()) {
					if(self_node == (*conn)->postNode()->getId()|| //input
							self_node == (*conn)->preNode()->getId()	) //output
							{
						scheduled = true;
						connectors.push((*conn)->getConnector());
						nextComm =  self_node == (*conn)->postNode()->getId() ? (*conn)->postNode()->localTime() :
								(*conn)->preNode()->localTime();
							}
					MUSIC_LOG0("Scheduled communication:"<< (*conn)->preNode()->getId() <<"->"<< (*conn)->postNode()->getId() << "at(" << (*conn)->preNode()->localTime().time() << ", "<< (*conn)->postNode()->localTime().time() <<")");
					(*conn)->advance();
				}
			}
		}
	}
}

/*

  void
  Synchronizer::setInterpolate (bool flag)
  {
    interpolate_ = flag;
  }


  // Start sampling (and fill the output buffers) at a time dependent
  // on latency and receiver's tick interval.  A negative latency can
  // delay start of sampling beyond time 0.  The tickInterval together
  // with the strict comparison has the purpose of supplying an
  // interpolating receiver side with samples.
  bool
  OutputSynchronizer::sample ()
  {
    return (localTime->integerTime () + latency_ + nextReceive.tickInterval ()
	    > 0);
  }

  // Return the number of copies of the data sampled by the sender
  // Runtime constructor which should be stored in the receiver
  // buffers at the first tick () (which occurs at the end of the
  // Runtime constructor)
  int
  InputSynchronizer::initialBufferedTicks ()
  {
    if (nextSend.tickInterval () < nextReceive.tickInterval ())
      {
	// InterpolatingOutputConnector - PlainInputConnector

	if (latency_ <= 0)
	  return 0;
	else
	  {
	    // Need to add a sample first when we pass the receiver
	    // tick (=> - 1).  If we haven't passed, the interpolator
	    // could simply use an interpolation coefficient of 0.0.
	    // (But this will never happen since that case isn't
	    // handled by an InterpolatingOutputConnector.)
	    int ticks = (latency_ - 1) / nextReceive.tickInterval ();

	    // Need to add a sample if we go outside of the sender
	    // interpolation window
	    if (latency_ >= nextSend.tickInterval ())
	      ticks += 1;

	    return ticks;
	  }
      }
    else
      {
	// PlainOutputConnector - InterpolatingInputConnector

	if (latency_ <= 0)
	  return 0;
	else
	  // Need to add a sample first when we pass the receiver
	  // tick (=> - 1).  If we haven't passed, the interpolator
	  // can simply use an interpolation coefficient of 0.0.
	  return 1 + (latency_ - 1) / nextReceive.tickInterval ();;
      }
  }


  // This function is only called when sender is remote
  void
  InterpolationSynchronizer::setSenderTickInterval (ClockState ti)
  {
    Synchronizer::setSenderTickInterval (ti);
    remoteTime.configure (localTime->timebase (), ti);
  }


  // This function is only called when receiver is remote
  void
  InterpolationSynchronizer::setReceiverTickInterval (ClockState ti)
  {
    Synchronizer::setReceiverTickInterval (ti);
    remoteTime.configure (localTime->timebase (), ti);
  }


  void
  InterpolationSynchronizer::remoteTick ()
  {
    remoteTime.tick ();
  }


  // Set the remoteTime which is used to control sampling and
  // interpolation.
  //
  // For positive latencies, the integer part (in terms of receiver
  // ticks) of the latency is handled by filling up the receiver
  // buffers using InputSynchronizer::initialBufferedTicks ().
  // remoteTime then holds the fractional part.
  void
  InterpolationOutputSynchronizer::initialize ()
  {
    if (latency_ > 0)
      {
	ClockState startTime = - latency_ % remoteTime.tickInterval ();
	if (latency_ >= localTime->tickInterval ())
	  startTime = startTime + remoteTime.tickInterval ();
	remoteTime.set (startTime);
      }
    else
      remoteTime.set (- latency_);
    Synchronizer::initialize ();
  }


   The order of execution in Runtime::tick () is:
 *
 * 1. localTime.tick ()
 * 2. Port::tick ()
 * 3. Connector::tick ()
 *     =>  call of Synchronizer::tick ()
 *         call of Synchronizer::sample ()
 * 4. Communication
 * 5. postCommunication
 *
 * After the last sample at 1 localTime will be between remoteTime
 * and remoteTime + localTime->tickInterval.  We trigger on this
 * situation and forward remoteTime.


  bool
  InterpolationOutputSynchronizer::sample ()
  {
    ClockState sampleWindowLow
      = remoteTime.integerTime () - localTime->tickInterval ();
    ClockState sampleWindowHigh
      = remoteTime.integerTime () + localTime->tickInterval ();
    return (sampleWindowLow <= localTime->integerTime ()
	    && localTime->integerTime () < sampleWindowHigh);
  }


  bool
  InterpolationOutputSynchronizer::interpolate ()
  {
    ClockState sampleWindowHigh
      = remoteTime.integerTime () + localTime->tickInterval ();
    return (remoteTime.integerTime () <= localTime->integerTime ()
	    && localTime->integerTime () < sampleWindowHigh);
  }


  double
  InterpolationOutputSynchronizer::interpolationCoefficient ()
  {
    ClockState prevSampleTime
      = localTime->integerTime () - localTime->tickInterval ();
    double c = ((double) (remoteTime.integerTime () - prevSampleTime)
		/ (double) localTime->tickInterval ());

    MUSIC_LOGR ("interpolationCoefficient = " << c);
    // NOTE: preliminary implementation which just provides
    // the functionality specified in the API
    if (interpolate_)
      return c;
    else
      return round (c);
  }


  void
  InterpolationOutputSynchronizer::tick ()
  {
    OutputSynchronizer::tick ();
  }


  void
  InterpolationInputSynchronizer::initialize ()
  {
    if (latency_ > 0)
      remoteTime.set (latency_ % remoteTime.tickInterval ()
		      - 2 * remoteTime.tickInterval ());
    else
      remoteTime.set (latency_ % remoteTime.tickInterval ()
		      - remoteTime.tickInterval ());
    Synchronizer::initialize ();
  }


  bool
  InterpolationInputSynchronizer::sample ()
  {
    return localTime->integerTime () > remoteTime.integerTime ();
  }


  double
  InterpolationInputSynchronizer::interpolationCoefficient ()
  {
    ClockState prevSampleTime
      = remoteTime.integerTime () - remoteTime.tickInterval ();
    double c = ((double) (localTime->integerTime () - prevSampleTime)
		/ (double) remoteTime.tickInterval ());

    MUSIC_LOGR ("interpolationCoefficient = " << c);
    // NOTE: preliminary implementation which just provides
    // the functionality specified in the API
    if (interpolate_)
      return c;
    else
      return round (c);
  }


  void
  InterpolationInputSynchronizer::tick ()
  {
    InputSynchronizer::tick ();
  }*/

}

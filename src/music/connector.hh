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
#include <music/event_router.hh>

#include <music/subconnector.hh>

namespace MUSIC {

  typedef char contDataT;

  // The connector is responsible for one side of the communication
  // between the ports of a port pair.  An output port can have
  // multiple connectors while an input port only has one.  The method
  // connector::connect () creates one subconnector for each MPI
  // process we will communicate with on the remote side.

  class Connector {
  protected:
    ConnectorInfo info;
    SpatialNegotiator* spatialNegotiator;
    MPI::Intracomm comm;
  public:
    Connector () { }
    Connector (ConnectorInfo _info,
	       SpatialNegotiator* _spatialNegotiator,
	       MPI::Intracomm c);
    std::string receiverAppName () const
    { return info.receiverAppName (); }
    std::string receiverPortName () const
    { return info.receiverPortName (); }
    int receiverPortCode () const
    { return info.receiverPortCode (); }
    int remoteLeader () const
    { return info.remoteLeader (); }
    int maxLocalWidth () { return spatialNegotiator->maxLocalWidth (); }
    MPI::Intercomm createIntercomm ();
    virtual void
    spatialNegotiation (std::vector<OutputSubconnector*>& osubconn,
			std::vector<InputSubconnector*>& isubconn) { }
    virtual void tick (bool& requestCommunication) = 0;
  };

  class OutputConnector : virtual public Connector {
  protected:
    OutputSynchronizer synch;
  public:
    OutputSynchronizer* synchronizer () { return &synch; }
    void tick (bool& requestCommunication);
  };
  
  class InputConnector : virtual public Connector {
  protected:
    InputSynchronizer synch;
  public:
    InputSynchronizer* synchronizer () { return &synch; }
    void tick (bool& requestCommunication);
  };

  class ContConnector : virtual public Connector {
  public:
    void swapBuffers (contDataT*& b1, contDataT*& b2);
  };  
  
  class FastConnector : virtual public Connector {
  protected:
    contDataT* prevSample;
    contDataT* sample;
  };
  
  class ContOutputConnector : public ContConnector, public OutputConnector {
  public:
    void send ();
  };
  
  class ContInputConnector : public ContConnector, public InputConnector {
  protected:
    void receive ();
  public:
  };
  
  class FastContOutputConnector : public ContOutputConnector,
				  public FastConnector {
    void interpolateTo (int start, int end, contDataT* data);
    void interpolateToBuffers ();
    void applicationTo (contDataT* data);
    void mark ();
  public:
    void tick ();
  };
  
  class SlowContInputConnector : public ContInputConnector {
  private:
    void buffersToApplication ();
    void toApplication ();
  public:
    void tick ();
  };
  
  class SlowContOutputConnector : public ContOutputConnector {
    void applicationToBuffers ();
  public:
    void tick ();
  };
  
  class FastContInputConnector : public ContInputConnector,
				 public FastConnector {
    void buffersTo (contDataT* data);
    void interpolateToApplication ();
  public:
    void tick ();
  };

  class EventConnector : virtual public Connector {
  public:
    EventConnector (ConnectorInfo info,
		    SpatialNegotiator* spatialNegotiator,
		    MPI::Intracomm c);
  };
  
  class EventOutputConnector : public OutputConnector, public EventConnector {
    EventRouter& router;
    void send ();
  public:
    EventOutputConnector (ConnectorInfo connInfo,
			  SpatialOutputNegotiator* spatialNegotiator,
			  MPI::Intracomm comm,
			  EventRouter& router);
    void spatialNegotiation (std::vector<OutputSubconnector*>& osubconn,
			     std::vector<InputSubconnector*>& isubconn);
    void tick ();
  };
  
  class EventInputConnector : public InputConnector, public EventConnector {
    
  private:
    EventHandlerPtr handleEvent;
    Index::Type type;
  public:
    EventInputConnector (ConnectorInfo connInfo,
			 SpatialInputNegotiator* spatialNegotiator,
			 EventHandlerPtr handleEvent,
			 Index::Type type,
			 MPI::Intracomm comm);
    void spatialNegotiation (std::vector<OutputSubconnector*>& osubconn,
			     std::vector<InputSubconnector*>& isubconn);
  };
  
}

#define MUSIC_CONNECTOR_HH
#endif

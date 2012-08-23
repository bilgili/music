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

#ifndef MUSIC_SCHEDULER_HH

//#define MUSIC_DEBUG
#include "music/music-config.hh"

#if MUSIC_USE_MPI

#include <limits>
#include <vector>
#include <music/clock.hh>
#include <music/connector.hh>

namespace MUSIC {

// The Scheduler is responsible for the timing involved in
// communication, sampling, interpolation and buffering.

  class Scheduler
  {

  public:
    class Connection;
    class Node {
      int id_;
      Clock localTime_;
      std::vector<Connection*> outputConnections_;
      std::vector<Connection*> inputConnections_;
    public:
      Node(int id, const Clock &localTime);
      void advance();
      void addConnection(Connection *conn, bool input = false);
      std::vector<Connection*>* outputConnections ()
      {
	return &outputConnections_;
      };
      Clock localTime () const {return localTime_;};
      double nextReceive() const;
      int getId() const {return id_;}
    };

    class Connection {
      Clock nextSend_;
      Clock nextReceive_;
      int pre_id,post_id;
      Node *pre_;
      Node *post_;
      ClockState latency_;
      int maxBuffered_;
      bool interpolate_;
      int port_code_;
      Connector *connector_;
    public:
      Connection(int pre,int post,const ClockState &latency,int maxBuffered,  bool interpolate, int port_code);
      void initialize(std::vector<Node*> &nodes);
      void advance();
      Clock nextSend() const {return nextSend_;}
      Clock nextReceive() const {return nextReceive_;}
      Connector *getConnector() const {return connector_;}
      void setConnector(Connector *conn) {connector_ = conn;}
      int portCode() const {return port_code_;}
      Node *preNode() const {return pre_;}
      Node *postNode() const {return post_;}
      ClockState getLatency()const{return latency_;}
      bool getInterpolate()const{return interpolate_;}
    private:
      void _advance();

    };


  private:
    std::vector<Node*> nodes;
    std::vector<Connection*>connections;
    int self_node;

  public:
    Scheduler(int node_id);
    ~Scheduler();
    void addNode(int id, const Clock &localTime);
    void addConnection(int pre_id,int post_id,const ClockState &latency, int maxBuffered, bool interpolate, int port_code);
    void initialize(std::vector<Connector*> &connectors);
    void nextCommunication (std::vector<std::pair<double, Connector *> > &schedule);
  };
/*
  class Synchronizer {

  protected:
    // pointer to the main clock of this application
    Clock* localTime;
    Scheduler *scheduler;

    // the following two clocks are mirrored on both peers; this is
    // the core of the mechanism by which MUSIC avoids handshaking

    // this clock holds the time (in the sender time frame) when an
    // OutputConnector is going to send next packet of data to its
    // peer InputConnector
    Clock nextSend;

    // this clock holds the time (in the receiver time frame) when an
    // InputConnector is going to receive next packet of data from its
    // peer OutputConnector
    Clock nextReceive;

    // for events and messages, this is how late data is permitted to
    // arrive at the receiver; for continous data this is how much
    // data is delayed; negative values are allowed
    ClockState latency_;

    // how much extra buffering is allowed; measured in units of
    // sender side ticks
    int maxBuffered_;

    // interpolate rather than picking value closest in time
    bool interpolate_;

    // cached decision to communicate; (nextSend or nextReceive time
    // has arrived)
    bool communicate_;

    virtual void nextCommunication (){};
  public:
    Synchronizer(){};
    virtual ~Synchronizer(){};
    void setLocalTime (Clock* lt);
    virtual void setSenderTickInterval (ClockState ti);
    virtual void setReceiverTickInterval (ClockState ti);
    void setMaxBuffered (int m);
    int allowedBuffered () { return maxBuffered_; }
    void setAccLatency (ClockState l);
    ClockState delay () { return latency_; }
    void setInterpolate (bool flag);
    virtual void initialize ();
    virtual int initialBufferedTicks () { return 0; };
    bool communicate ();
    virtual void tick ()=0;
    double time ()
      {
        return localTime->time ();
      }
  };


  class OutputSynchronizer : virtual public Synchronizer {
  public:
    bool sample ();
    void tick ();
  protected:
    void nextCommunication (){};
  };


  class InputSynchronizer : virtual public Synchronizer {
  public:
    virtual int initialBufferedTicks ();
    void tick ();
  protected:
    void nextCommunication (){};
  };


  class InterpolationSynchronizer : virtual public Synchronizer {
  protected:
    Clock remoteTime;
  public:
    virtual void setSenderTickInterval (ClockState ti);
    virtual void setReceiverTickInterval (ClockState ti);    
    void remoteTick ();
  };


  class InterpolationOutputSynchronizer : public InterpolationSynchronizer,
					  public OutputSynchronizer {
  public:
    void initialize ();
    bool sample ();
    bool interpolate ();
    double interpolationCoefficient ();
    void tick ();
  };


  class InterpolationInputSynchronizer : public InterpolationSynchronizer,
					 public InputSynchronizer {
  public:
    void initialize ();
    bool sample ();
    double interpolationCoefficient ();
    void tick ();
  };
 */

}
#endif

#define MUSIC_SCHEDULER_HH
#endif

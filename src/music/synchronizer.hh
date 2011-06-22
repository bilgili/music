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

#ifndef MUSIC_SYNCHRONIZER_HH

#include <music/clock.hh>

namespace MUSIC {

  // The Synchronizer is responsible for the timing involved in
  // communication, sampling, interpolation and buffering.  There is
  // one Synchronizer in each Connector.  The Subconnectors of a
  // Connector also have a reference to the Connector's synchronizer.

  class Synchronizer {
  protected:
    // pointer to the main clock of this application
    Clock* localTime;

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
    
    void nextCommunication ();
  public:
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
  };


  class OutputSynchronizer : virtual public Synchronizer {
  public:
    bool sample ();
    void tick ();
  };


  class InputSynchronizer : virtual public Synchronizer {
  public:
    virtual int initialBufferedTicks ();
    void tick ();
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

}

#define MUSIC_SYNCHRONIZER_HH
#endif

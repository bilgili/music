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
#include "music/debug.hh" // Must be included first on BG/L

#include <cmath>

#include "music/synchronizer.hh"

namespace MUSIC {

  void
  Synchronizer::nextCommunication ()
  {
    // Advance receive time as much as possible
    // still ensuring that oldest data arrives in time
    ClockState limit
      = nextSend.integerTime () + latency_ - nextReceive.tickInterval ();
    while (nextReceive.integerTime () <= limit)
      nextReceive.tick ();
    // Advance send time to match receive time
    limit = nextReceive.integerTime () + nextReceive.tickInterval () - latency_;
    int bCount = 0;
    while (nextSend.integerTime () <= limit)
      {
	nextSend.tick ();
	++bCount;
      }
    // Advance send time according to precalculated buffer
    if (bCount < maxBuffered_)
      nextSend.ticks (maxBuffered_ - bCount);
    MUSIC_LOGR ("next send at " << nextSend.time ()
		<< ", next receive at " << nextReceive.time ());
  }

  
  void
  Synchronizer::setLocalTime (Clock* lt)
  {
    localTime = lt;
    nextSend.configure (localTime->timebase (), localTime->tickInterval ());
    nextReceive.configure (localTime->timebase (), localTime->tickInterval ());
  }

  
  void
  Synchronizer::setSenderTickInterval (ClockState ti)
  {
    MUSIC_LOGR ("sender tick interval set to " << nextSend.timebase () * ti);
    nextSend.setTickInterval (ti);
  }

  
  void
  Synchronizer::setReceiverTickInterval (ClockState ti)
  {
    MUSIC_LOGR ("receiver tick interval set to "
		<< nextReceive.timebase () * ti);
    nextReceive.setTickInterval (ti);
  }

  
  void
  Synchronizer::setMaxBuffered (int m)
  {
    MUSIC_LOGR ("maxBuffered set to " << m);
    maxBuffered_ = m - 1;
  }


  void
  Synchronizer::setAccLatency (ClockState l)
  {
    MUSIC_LOGR ("accLatency set to " << nextReceive.timebase () * l);
    latency_ = l;
  }

  
  void
  Synchronizer::setInterpolate (bool flag)
  {
    interpolate_ = flag;
  }


  void
  Synchronizer::setMaxDelay (ClockState maxDelay)
  {
    setMaxDelay (maxDelay, *localTime);
    setMaxDelay (maxDelay, nextSend);
    setMaxDelay (maxDelay, nextReceive);
    localTime->ticks (-1);
  }


  void
  Synchronizer::setMaxDelay (ClockState maxDelay, Clock& clock)
  {
    // setup start time
    int delayedTicks = maxDelay / clock.tickInterval ();
    clock.ticks (- delayedTicks);
  }
  
  
  void
  Synchronizer::initialize ()
  {
    nextCommunication ();
  }

  
  bool
  Synchronizer::communicate ()
  {
    return communicate_;
  }


  bool
  Synchronizer::simulating ()
  {
    return localTime->integerTime () >= 0;
  }


  void
  OutputSynchronizer::tick ()
  {
    if (*localTime > nextSend)
      nextCommunication ();
    communicate_ = *localTime == nextSend;
  }

  
  void
  InputSynchronizer::tick ()
  {
    if (*localTime > nextReceive)
      nextCommunication ();
    communicate_ = *localTime == nextReceive;
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
  InterpolationSynchronizer::tick ()
  {
    /* The order of execution in Runtime::tick () is:
     *
     * 1. Port::tick ()
     * 2. Connector::tick ()
     *     =>  call of Synchronizer::sample ()
     *         call of Synchronizer::tick ()
     * 3. localTime.tick ()
     *
     * After the last sample at 1 localTime will be between remoteTime
     * and remoteTime + localTime->tickInterval.  We trigger on this
     * situation and forward remoteTime.
     */ 
    if (localTime->integerTime () >= remoteTime.integerTime ())
      remoteTime.tick ();
  }


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
    
    // *fixme* preliminary implementation which just provides
    // the functionality specified in the API
    if (interpolate_)
      return c;
    else
      return round (c);
  }


  void
  InterpolationOutputSynchronizer::tick ()
  {
    InterpolationSynchronizer::tick ();
    OutputSynchronizer::tick ();
  }

  
  bool
  InterpolationInputSynchronizer::sample ()
  {
    ClockState sampleWindowLow
      = remoteTime.integerTime () - localTime->tickInterval ();
    double tb = localTime->timebase ();
    bool sample = (sampleWindowLow <= localTime->integerTime ());
    return sample;
  }


  double
  InterpolationInputSynchronizer::interpolationCoefficient ()
  {
    ClockState prevSampleTime
      = remoteTime.integerTime () - remoteTime.tickInterval ();
    double c = ((double) (localTime->integerTime () - prevSampleTime)
		/ (double) remoteTime.tickInterval ());
    
    // *fixme* preliminary implementation which just provides
    // the functionality specified in the API
    if (interpolate_)
      return c;
    else
      return round (c);
  }


  void
  InterpolationInputSynchronizer::tick ()
  {
    InterpolationSynchronizer::tick ();
    InputSynchronizer::tick ();
  }
  
}

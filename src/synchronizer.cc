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
#include <iostream>
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
    MUSIC_LOGR ("timebase = " << localTime->timebase ()
		<< ", ti = " << localTime->tickInterval ());
  }

  
  void
  Synchronizer::setSenderTickInterval (ClockState ti)
  {
    nextSend.setTickInterval (ti);
    MUSIC_LOGR ("nextSend.ti := " << ti);
  }

  
  void
  Synchronizer::setReceiverTickInterval (ClockState ti)
  {
    nextReceive.setTickInterval (ti);
    MUSIC_LOGR ("nextReceive.ti := " << ti);
  }

  
  void
  Synchronizer::setMaxBuffered (int m)
  {
    maxBuffered_ = m;
    MUSIC_LOGR ("maxBuffered_ := " << m);
  }


  void
  Synchronizer::setAccLatency (ClockState l)
  {
    latency_ = l;
    MUSIC_LOGR ("latency_ := " << l);
  }

  
  void
  Synchronizer::setInterpolate (bool flag)
  {
    interpolate_ = flag;
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
  OutputSynchronizer::sample ()
  {
    return (localTime->integerTime () + latency_ + nextReceive.tickInterval ()
	    > 0);
  }

  
  void
  OutputSynchronizer::tick ()
  {
    if (*localTime > nextSend)
      nextCommunication ();
    communicate_ = *localTime == nextSend;
  }


  int
  InputSynchronizer::initialBufferedTicks ()
  {
    if (nextSend.tickInterval () < nextReceive.tickInterval ())
      {
	if (latency_ <= 0)
	  return 0;
	else
	  {
	    int ticks = (latency_ - 1) / nextReceive.tickInterval ();
	    if (latency_ >= nextSend.tickInterval ())
	      ticks += 1;
	    return ticks;
	  }
      }
    else
      {
	if (latency_ <= 0)
	  return 0;
	else
	  return 1 + (latency_ - 1) / nextReceive.tickInterval ();;
      }
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
  InterpolationSynchronizer::remoteTick ()
  {
    remoteTime.tick ();
  }


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

  
  /* The order of execution in Runtime::tick () is:
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
   */
  
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
  }
  
}

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

#include "music/synchronizer.hh"

namespace MUSIC {

  void
  Synchronizer::nextCommunication ()
  {
    // Advance receive time as much as possible
    // still ensuring that oldest data arrives in time
    ClockStateT limit
      = nextSend.integerTime () + latency - nextReceive.tickInterval ();
    while (nextReceive.integerTime () <= limit)
      nextReceive.tick ();
    // Advance send time to match receive time
    limit = nextReceive.integerTime () + nextReceive.tickInterval () - latency;
    int bCount = 0;
    while (nextSend.integerTime () <= limit)
      {
	nextSend.tick ();
	++bCount;
      }
    // Advance send time according to precalculated buffer
    if (bCount < maxBuffered)
      nextSend.ticks (maxBuffered - bCount);
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
  Synchronizer::setSenderTickInterval (ClockStateT ti)
  {
    MUSIC_LOGR ("sender tick interval set to " << nextSend.timebase () * ti);
    nextSend.setTickInterval (ti);
  }

  
  void
  Synchronizer::setReceiverTickInterval (ClockStateT ti)
  {
    MUSIC_LOGR ("receiver tick interval set to "
		<< nextReceive.timebase () * ti);
    nextReceive.setTickInterval (ti);
  }

  
  void
  Synchronizer::setMaxBuffered (int m)
  {
    MUSIC_LOGR ("maxBuffered set to " << m);
    maxBuffered = m - 1;
  }


  void
  Synchronizer::setAccLatency (ClockStateT l)
  {
    MUSIC_LOGR ("accLatency set to " << nextReceive.timebase () * l);
    latency = l;
  }

  
  void
  Synchronizer::initialize ()
  {
    nextCommunication ();
  }

  
  bool
  Synchronizer::sample ()
  {
    return true;
  }


  bool
  Synchronizer::mark ()
  {
    return true;
  }


  bool
  Synchronizer::communicate ()
  {
    return communicate_;
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
  
}

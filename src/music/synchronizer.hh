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

  class Synchronizer {
#if 1
  public:
#else
  protected:
#endif
    Clock* localTime;
    Clock nextSend;
    Clock nextReceive;
    ClockStateT latency;
    int maxBuffered;
    bool _communicate;
    void nextCommunication ();
  public:
    void setLocalTime (Clock* lt);
    void setSenderTickInterval (ClockStateT ti);
    void setReceiverTickInterval (ClockStateT ti);
    // algorithm expects *extra* buffered ticks so we subtract 1
    void setMaxBuffered (int m);
    void setAccLatency (ClockStateT l) { latency = l; }
    bool sample ();
    bool mark ();
    bool communicate ();
  };


  class OutputSynchronizer : public Synchronizer {
  public:
    void tick ();
  };


  class InputSynchronizer : public Synchronizer {
  public:
    void tick ();
  };

}

#define MUSIC_SYNCHRONIZER_HH
#endif

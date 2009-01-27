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

#ifndef MUSIC_CLOCK_HH

#include "music/music-config.hh"

namespace MUSIC {

  // ClockStateT may be negative due to calculations in Synchronizer
#ifdef MUSIC_HAVE_LONG_LONG
  typedef long long ClockStateT;
#else
#error 64-bit clocks without long long not yet implemented
#endif
  
  class Clock {
    ClockStateT state_;
    ClockStateT tickInterval_;
    double timebase_;
  public:
    Clock () { };
    Clock (double tb, double h);
    void tick ();
    void ticks (int n);
    ClockStateT tickInterval () { return tickInterval_; }
    double timebase () { return timebase_; }
    double time ();
    ClockStateT integerTime () { return state_; }
    bool operator> (const Clock& ref) const { return state_ > ref.state_; }
    bool operator== (const Clock& ref) const { return state_ == ref.state_; }
  };

}

#define MUSIC_CLOCK_HH
#endif

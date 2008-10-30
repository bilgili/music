/*
 *  This file is part of MUSIC.
 *  Copyright (C) 2008 INCF
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

#ifdef MUSIC_HAVE_LONG_LONG
  typedef unsigned long long clock_state_t;
#else
#error 64-bit clocks without long long not yet implemented
#endif
  
  class clock {
    clock_state_t state;
    clock_state_t _tick_interval;
    double timebase;
  public:
    clock () { };
    clock (double tb, double h);
    void tick ();
    clock_state_t tick_interval () { return _tick_interval; }
    double time ();
  };

}

#define MUSIC_CLOCK_HH
#endif

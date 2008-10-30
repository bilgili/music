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

#include "music/clock.hh"

namespace MUSIC {

  clock::clock (double tb, double h)
    : timebase (tb)
  {
    state = 0;
    _tick_interval = (unsigned long long) (h / tb + 0.5);
  }

  
  void
  clock::tick ()
  {
    state += _tick_interval;
  }
  

  double
  clock::time ()
  {
    return timebase * state;
  }
  
}

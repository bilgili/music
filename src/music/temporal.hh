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

#ifndef MUSIC_TEMPORAL_HH

#include <music/index_map.hh>
#include <music/clock.hh>

namespace MUSIC {

  class setup;

  class temporal_negotiation_data {
    index_interval _interval;
    int _rank;
  public:
    temporal_negotiation_data (index_interval i, int r)
      : _interval (i), _rank (r) { }
  };

  class temporal_negotiator {
  public:
    temporal_negotiator (setup* s, double timebase, clock_state_t ti);
    void negotiate () { };
  };

}

#define MUSIC_TEMPORAL_HH
#endif

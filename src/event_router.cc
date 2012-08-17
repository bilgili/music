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

#include <iostream>
#include "music/debug.hh"
#include "music/event.hh"
#include "music/event_router.hh"

namespace MUSIC {

  OutputRoutingData::OutputRoutingData (const IndexInterval &i, FIBO* b) : EventRoutingData(i), buffer_ (b)
  {
  }

  void
  OutputRoutingData::process (double t, int id)
  {
    Event* e = static_cast<Event*> (buffer_->insert ());
    e->t = t;
    e->id = id;
  }

}

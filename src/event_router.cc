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

#include "music/event_router.hh"
#include "music/event.hh"

namespace MUSIC {

  event_router::event_router (FIBO* buffer)
  {
    buffers.push_back (buffer);
  }
  
  
  void
  event_router::insert_event (double t, int id)
  {
    //*fixme*
    event* e = static_cast<event*> (buffers[0]->insert ());
    e->t = t;
    e->id = id;
  }

}
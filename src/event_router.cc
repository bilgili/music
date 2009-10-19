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
#include "music/debug.hh"

#include "music/event_router.hh"
#include "music/event.hh"

namespace MUSIC {
  
  void
  EventRouter::insertRoutingInterval (EventRoutingData& data)
  {
    routingTable.add (data);
  }
  

  void
  EventRouter::insertRoutingInterval (IndexInterval i, FIBO* b)
  {
    routingTable.add (EventRoutingData (i, b));
  }
  

  void
  EventRouter::buildTable ()
  {
    MUSIC_LOG0 ("Routing table size for rank 0 = " << routingTable.size ());
    routingTable.build ();
  }


  void
  EventRouter::insertEvent (double t, GlobalIndex id)
  {
    Inserter i (t, id);
    routingTable.search (id, &i);
  }

  void
  EventRouter::insertEvent (double t, LocalIndex id)
  {
    Inserter i (t, id);
    routingTable.search (id, &i);
  }

}

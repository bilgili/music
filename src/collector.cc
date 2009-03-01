/*
 *  This file is part of MUSIC.
 *  Copyright (C) 2009 INCF
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

#include <algorithm>

#include "music/event.hh"

#include "music/collector.hh"

namespace MUSIC {

  Collector::Interval::Interval (IndexInterval& interval)
  {
    setBegin (interval.begin () - interval.local ());
    setLength (interval.end () - interval.begin());
  }
  

  void
  Collector::configure (DataMap* dmap, int allowedBuffered)
  {
    MUSIC_LOG ("Collector::configure");
    dataMap = dmap;
    allowedBuffered_ = allowedBuffered;
  }
  

  void
  Collector::addRoutingInterval (IndexInterval interval, BIFO* buffer)
  {
    MUSIC_LOG ("Collector::addRoutingInterval");
    BufferMap::iterator b = buffers.find (buffer);
    if (b == buffers.end ())
      {
	buffers.insert (std::make_pair (buffer, Intervals ()));
	b = buffers.find (buffer);
      }
    Intervals& intervals = b->second;
    intervals.push_back (Interval (interval));
  }


  void
  Collector::initialize ()
  {
    MUSIC_LOG ("Collector::initialize");
    for (BufferMap::iterator b = buffers.begin (); b != buffers.end (); ++b)
      {
	BIFO* buffer = b->first;
	Intervals& intervals = b->second;
	sort (intervals.begin (), intervals.end ());
	int elementSize = dataMap->type ().Get_size ();
	MUSIC_LOG ("elementSize = " << elementSize);
	int size = 0;
	for (Intervals::iterator i = intervals.begin ();
	     i != intervals.end ();
	     ++i)
	  {
	    MUSIC_LOG ("len = " << i->length ());
	    int length = elementSize * i->length ();
	    i->setLength (length);
	    size += length;
	  }
	buffer->configure (size, size * (allowedBuffered_ + 1));
      }
  }


  void
  Collector::collect (ContDataT* base)
  {
    for (BufferMap::iterator b = buffers.begin (); b != buffers.end (); ++b)
      {
	BIFO* buffer = b->first;
	Intervals& intervals = b->second;
	ContDataT* src = static_cast<ContDataT*> (buffer->next ());
	ContDataT* dest = static_cast<ContDataT*> (base);
	int elementSize = dataMap->type ().Get_size ();
	for (Intervals::iterator i = intervals.begin ();
	     i != intervals.end ();
	     ++i)
	  {
	    memcpy (dest + i->begin (), src, i->length ());
	    dest += i->length ();
	  }
      }
  }

  void
  Collector::collect ()
  {
    collect (static_cast<ContDataT*> (dataMap->base ()));
  }

}

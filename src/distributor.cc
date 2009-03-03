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

#include "music/distributor.hh"

namespace MUSIC {

  Distributor::Interval::Interval (IndexInterval& interval)
  {
    setBegin (interval.begin ());
    setLength (interval.end () - interval.begin ());
  }
  

  void
  Distributor::configure (DataMap* dmap)
  {
    dataMap = dmap;
  }

  
  IntervalTree<int, IndexInterval>*
  Distributor::buildTree ()
  {
    IntervalTree<int, IndexInterval>* tree
      = new IntervalTree<int, IndexInterval> ();
    
    IndexMap* indices = dataMap->indexMap ();
    for (IndexMap::iterator i = indices->begin ();
	 i != indices->end ();
	 ++i)
      {
	MUSIC_LOGR ("adding (" << i->begin () << ", " << i->end ()
		    << ", " << i->local () << ") to tree");
	tree->add (*i);
      }

    tree->build ();
    
    return tree;
  }
  

  void
  Distributor::addRoutingInterval (IndexInterval interval, FIBO* buffer)
  {
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
  Distributor::IntervalCalculator::operator() (IndexInterval& indexInterval)
  {
    MUSIC_LOG ("action!");
    interval_.setBegin (elementSize_
			* (interval_.begin () - indexInterval.local ()));
    interval_.setLength (elementSize_ * interval_.length ());
    MUSIC_LOG ("end!");
  }


  void
  Distributor::initialize ()
  {
    IntervalTree<int, IndexInterval>* tree = buildTree ();
    
    for (BufferMap::iterator b = buffers.begin (); b != buffers.end (); ++b)
      {
	FIBO* buffer = b->first;
	Intervals& intervals = b->second;
	sort (intervals.begin (), intervals.end ());
	int elementSize = dataMap->type ().Get_size ();
	int size = 0;
	for (Intervals::iterator i = intervals.begin ();
	     i != intervals.end ();
	     ++i)
	  {
	    IntervalCalculator calculator (*i, elementSize);
	    tree->search (i->begin (), &calculator);
	    size += i->length ();
	  }
	buffer->configure (size);
      }

    delete tree;
  }


  void
  Distributor::distribute ()
  {
    for (BufferMap::iterator b = buffers.begin (); b != buffers.end (); ++b)
      {
	FIBO* buffer = b->first;
	Intervals& intervals = b->second;
	ContDataT* src = static_cast<ContDataT*> (dataMap->base ());
	ContDataT* dest = static_cast<ContDataT*> (buffer->insert ());
	for (Intervals::iterator i = intervals.begin ();
	     i != intervals.end ();
	     ++i)
	  {
	    MUSIC_LOGR ("src = " << static_cast<void*> (src)
		       << ", begin = " << i->begin ()
		       << ", length = " << i->length ());
	    memcpy (dest, src + i->begin (), i->length ());
	    dest += i->length ();
	  }
      }
  }
  
}

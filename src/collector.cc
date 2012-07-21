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
#include "music/collector.hh"

#ifdef USE_MPI
// collector.hh needs to be included first since it causes inclusion
// of mpi.h (in data_map.hh).  mpi.h must be included before other
// header files on BG/L


#include <algorithm>
#include <cstring>

#include "music/event.hh"

namespace MUSIC {

  Collector::Interval::Interval (IndexInterval& interval)
  {
    setBegin (interval.begin ());
    setLength (interval.end () - interval.begin ());
  }
  

  void
  Collector::configure (DataMap* dmap, int maxsize)
  {
    dataMap = dmap;
    maxsize_ = maxsize;
  }
  

  IntervalTree<int>*
  Collector::buildTree ()
  {
    IntervalTree<int>* tree
      = new IntervalTree<int> ();
    
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
  Collector::addRoutingInterval (IndexInterval interval, BIFO* buffer)
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
  Collector::IntervalCalculator::operator() ( MUSIC::Interval& indexInterval)
  {
    interval_.setBegin (elementSize_
			* (interval_.begin () - ((IndexInterval&)indexInterval).local ()));
    interval_.setLength (elementSize_ * interval_.length ());
  }


  void
  Collector::initialize ()
  {
    IntervalTree<int>* tree = buildTree ();
    for (BufferMap::iterator b = buffers.begin (); b != buffers.end (); ++b)
      {
	BIFO* buffer = b->first;
	Intervals& intervals = b->second;
	sort (intervals.begin (), intervals.end ());
	int elementSize = dataMap->type ().Get_size ();
	int size = 0;
	for (Intervals::iterator i = intervals.begin ();
	     i != intervals.end ();
	     ++i)
	  {
	    IntervalCalculator calculator (*i, elementSize);
	    MUSIC_LOGX ("searching for " << i->begin ());
	    tree->search (i->begin (), &calculator);
	    size += i->length ();
	   // std::cerr << MPI::COMM_WORLD.Get_rank() << " beg:" << i->begin() << " end:" << i->length() << std::endl;
	  }
	//buffer->configure (size, size * allowedBuffered_);
	// std::cerr << MPI::COMM_WORLD.Get_rank() <<size << ":" << maxsize_ << std::endl;
	buffer->configure (size, maxsize_);
      }
  }


  void
  Collector::collect (ContDataT* base)
  {
	ContDataT* dest = static_cast<ContDataT*> (base);
    for (BufferMap::iterator b = buffers.begin (); b != buffers.end (); ++b)
      {
	BIFO* buffer = b->first;
	Intervals& intervals = b->second;
	ContDataT* src = static_cast<ContDataT*> (buffer->next ());
	if (src == NULL)
	  // Out of data (remote has flushed)
	  return;
	for (Intervals::iterator i = intervals.begin ();
	     i != intervals.end ();
	     ++i)
	  {
	    MUSIC_LOGX ("collect to dest = " << static_cast<void*> (dest)
		       << ", begin = " << i->begin ()
		       << ", length = " << i->length ());

	    memcpy (dest + i->begin (), src, i->length ());
	    src += i->length ();
	  }
      }
  }

  void
  Collector::collect ()
  {
    collect (static_cast<ContDataT*> (dataMap->base ()));
  }

}
#endif

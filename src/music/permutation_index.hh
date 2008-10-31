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

#ifndef MUSIC_PERMUTATION_INDEX_HH

#include <vector>

#include "music/index_map.hh"

namespace MUSIC {

  class permutation_index : public index_map {
    std::vector<index_interval> _indices;
  public:
    class iterator : public index_map::iterator_implementation {
      const index_interval* interval_ptr;
    public:
      iterator (const index_interval* ptr) : interval_ptr (ptr) { }
      virtual const index_interval operator* () { return *interval_ptr; }
      virtual const index_interval* dereference () { return interval_ptr; }
      virtual bool is_equal (iterator_implementation* i) const
      {
	return interval_ptr == static_cast<iterator*> (i)->interval_ptr;
      }
      virtual void operator++ () { ++interval_ptr; }
      virtual iterator_implementation* copy ()
      {
	return new iterator (interval_ptr);
      }
    };
    
    permutation_index (global_index *indices, int size);
    permutation_index (std::vector<index_interval>& indices);
    virtual index_map::iterator begin ();
    virtual const index_map::iterator end () const;
    virtual index_map* copy ();    
  };

}

#define MUSIC_PERMUTATION_INDEX_HH
#endif

/*
 *  This file is part of MUSIC.
 *  Copyright (C) 2008 CSC, KTH
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

#include <mpi.h>
#include "music/setup.hh"
#include "music/runtime.hh"
#include "music/permutation_index.hh"

namespace MUSIC {
  
  bool operator< (const index_interval& a, const index_interval& b)
  {
    return (a.begin () < b.begin ()
	    || (a.begin () == b.begin () && a.end () < b.end ()));
  }
  
  permutation_index::permutation_index (global_index* indices, int size)
  {
    
    //*fixme* collapse where possible
    for (int i = 0; i < size; ++i)
      _indices.push_back (index_interval (indices[i],
					  indices[i] + 1,
					  indices[i] - i));
    sort (_indices.begin (), _indices.end ());
  }
  

  permutation_index::permutation_index (std::vector<index_interval>& indices)
    : _indices (indices)
  {
  }

  
  index_map::iterator
  permutation_index::begin ()
  {
    return index_map::iterator (new iterator (&_indices.front ()));
  }

  
  const index_map::iterator
  permutation_index::end () const
  {
    return index_map::iterator (new iterator (&_indices.back () + 1));
  }


  index_map*
  permutation_index::copy ()
  {
    return new permutation_index (_indices);
  }

}

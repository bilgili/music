/*
 *  This file is part of MUSIC.
 *  Copyright (C) 2007, 2008 CSC, KTH
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

namespace MUSIC {
  
  linear_index::iterator::iterator (linear_index* li)
    : _indices (li)
  {
  }


  const index_interval
  linear_index::iterator::operator* ()
  {
    return _indices->_interval;
  }


  const index_interval*
  linear_index::iterator::dereference ()
  {
    return &_indices->_interval;
  }


  void
  linear_index::iterator::operator++ ()
  {
    _indices = 0;
  }


  bool
  linear_index::iterator::is_equal (iterator_implementation* i) const
  {
    return _indices == static_cast<iterator*> (i)->_indices;
  }
  
  
  linear_index::linear_index (global_index baseindex, int size)
    : _interval (baseindex, baseindex + size, static_cast<int> (baseindex))
  {
  }


  index_map::iterator
  linear_index::begin ()
  {
    return index_map::iterator (new iterator (this));
  }

  
  const index_map::iterator
  linear_index::end () const
  {
    return index_map::iterator (new iterator (0));
  }


  index_map*
  linear_index::copy ()
  {
    return new linear_index (*this);
  }
  
}

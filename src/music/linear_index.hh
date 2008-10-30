/*
 *  This file is part of MUSIC.
 *  Copyright (C) 2007, 2008 INCF
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

#ifndef MUSIC_LINEAR_INDEX_HH

#include "music/index_map.hh"

namespace MUSIC {

  class linear_index : public index_map {
    index_interval _interval;
  public:
    class iterator : public index_map::iterator_implementation {
      linear_index* _indices;
    public:
      iterator (linear_index* li);
      virtual const index_interval operator* ();
      virtual const index_interval* dereference ();
      virtual bool is_equal (iterator_implementation* i) const;
      virtual void operator++ ();
    };

    linear_index (global_index baseindex, int size);
    //global_index base_index () const { return _baseindex; }
    //int size () const { return _size; }
    virtual index_map::iterator begin ();
    virtual const index_map::iterator end () const;
    virtual index_map* copy ();
  };

}

#define MUSIC_LINEAR_INDEX_HH
#endif

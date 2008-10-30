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

#include "music/index_map.hh"

namespace MUSIC {

  const index_interval
  index_map::iterator::operator* ()
  {
    return **_implementation;
  }
  

  const index_interval*
  index_map::iterator::operator-> ()
  {
    return _implementation->dereference ();
  }
  

  bool
  index_map::iterator::operator== (const iterator& i) const
  {
    return _implementation->is_equal (i.implementation ());
  }
  

  bool
  index_map::iterator::operator!= (const iterator& i) const
  {
    return !_implementation->is_equal (i.implementation ());
  }
  

  index_map::iterator&
  index_map::iterator::operator++ ()
  {
    ++*_implementation;
    return *this;
  }

}

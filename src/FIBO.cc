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

#include "music/FIBO.hh"

namespace MUSIC {

  FIBO::FIBO (int es)
    : element_size (es)
  {
    size = element_size * n_initial;
    buffer.reserve (size);
    insertion = 0;
  }

  
  void *
  FIBO::insert ()
  {
    if (insertion == size)
      grow (2 * size);
    // Here we use the assumption that vector memory is contiguous
    // Josuttis says this is the intention of STL even though the
    // first version of the report is not clear about this.
    void* memory = static_cast<void*> (&buffer[insertion]);
    insertion += element_size;
    return memory;
  }
  

  void
  FIBO::mark ()
  {
  }


  void
  FIBO::next_block (void*& data, int& size)
  {
    //*fixme*
    data = static_cast<void*> (&buffer[0]);
    size = insertion;
    insertion = 0;
  }


  void
  FIBO::grow (int new_size)
  {
    size = new_size;
    buffer.reserve (size);
  }
  
}

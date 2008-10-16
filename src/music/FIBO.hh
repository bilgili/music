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

#ifndef MUSIC_FIBO_HH

#include <vector>

namespace MUSIC {

  class FIBO {
  private:
    static const int n_initial = 10;
    
    std::vector<char> buffer;
    int element_size;
    int size;
    int insertion;

    void grow (int new_size);
    
  public:
    FIBO () { }
    FIBO (int element_size);
    //*fixme* return type
    void* insert ();
    void mark ();
    void next_block (void*& data, int& size);
  };
  
  
}

#define MUSIC_FIBO_HH
#endif

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

#ifndef MUSIC_BIFO_HH

#include <vector>

#include <music/FIBO.hh>

namespace MUSIC {

  class BIFO {
  private:
    std::vector<char> buffer;
    int elementSize_;
    int size;			// size of buffer
    int beginning;		// beginning of last block
    int end;			// end of last block
    int top;			// upper bound of valid data
    int current;

    void grow (int newSize);
    
    int maxBlockSize_;
  public:
    BIFO () { }
    BIFO (int elementSize, int maxBlockSize);
    void configure (int elementSize, int maxBlockSize);
    bool isEmpty ();
    //*fixme* return type
    void* insertBlock ();
    // size in bytes
    void trimBlock (int size);
    void* next ();
  };
  
  
}

#define MUSIC_BIFO_HH
#endif

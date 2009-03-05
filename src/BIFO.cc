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

#include "music/error.hh"

#include "music/BIFO.hh"

namespace MUSIC {

  void
  BIFO::configure (int elementSize, int maxBlockSize)
  {
    elementSize_ = elementSize;
    maxBlockSize_ = maxBlockSize;
    size = maxBlockSize_;
    buffer.resize (size);
    beginning = 0;
    end = 0;
    current = 0;
    top = 0;
  }

  
  bool
  BIFO::isEmpty ()
  {
    return current == end;
  }


  void*
  BIFO::insertBlock ()
  {
    if (current <= end && current >= maxBlockSize_)
      beginning = 0;
    else
      {
	beginning = end;
	if (beginning + maxBlockSize_ > size)
	  grow (beginning + maxBlockSize_);
      }
    return static_cast<void*> (&buffer[beginning]);
  }


  void
  BIFO::trimBlock (int blockSize)
  {
    if (blockSize > 0)
      {
	end = beginning + blockSize;
	if (end > size)
	  error ("BIFO buffer overflow");
	if (current <= end)
	  top = end;
      }
  }


  void*
  BIFO::next ()
  {
    if (isEmpty ())
      {
	MUSIC_LOGR ("attempt to read from empty BIFO buffer");
	return NULL;
      }
    if (current == top)
      {
	// wrap around
	current = 0;
	top = end;
      }
    void* memory = static_cast<void*> (&buffer[current]);
    current += elementSize_;
    return memory;
  }

  void
  BIFO::grow (int newSize)
  {
    size = newSize;
    buffer.resize (size);
  }
    
}

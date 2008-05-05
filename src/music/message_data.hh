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

#ifndef MUSIC_MESSAGE_DATA_HH

#include "music/message.hh"
#include "music/data_map.hh"
#include "music/index_map.hh"

namespace MUSIC {

  class message_data : public data_map {
  public:
    message_data (message_handler* handler, index_map* map);
    message_data (int base_index, int size);
  };

}

#define MUSIC_MESSAGE_DATA_HH
#endif

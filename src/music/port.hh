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

#ifndef MUSIC_PORT_HH

#include "music/event_data.hh"
#include "music/message_data.hh"

namespace MUSIC {

  class port {
  public:
    virtual bool is_connected ();
    virtual int size ();
    virtual void map (data_map* m);
  };

  class output_port : public port {
  };

  class input_port : public port {
  };

  class cont_port : public port {
  };

  class cont_output_port : public cont_port, public output_port {
  };
  
  class cont_input_port : public cont_port, public input_port {
  };
  
  class event_port : public port {
  };

  class event_output_port : public event_port, public output_port {
  public:
    event_output_port (event_data* map);
  };

  class event_input_port : public event_port, public input_port {
  public:
    event_input_port (event_data* map);
  };

  class message_port : public port {
  };

  class message_output_port : public message_port, public output_port {
  public:
    message_output_port (message_data* map);
  };

  class message_input_port : public message_port, public input_port {
  public:
    message_input_port (message_data* map);
  };

}

#define MUSIC_PORT_HH
#endif

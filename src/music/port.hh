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

#include <string>

#include <music/data_map.hh>
#include <music/index_map.hh>
#include <music/event.hh>
#include <music/message.hh>
#include <music/event_router.hh>
#include <music/connectivity.hh>

namespace MUSIC {

  class setup;

  class port {
    static const int constant_width = 60;
    int _width;
  protected:
    setup* _setup;
    void check_connected ();
  public:
    port () { }
    port (setup* s, std::string identifier) : _setup (s), _width (constant_width) { }
    bool is_connected ();
    bool has_width ();
    int width ();
  };

  class output_port : virtual public port {
  };

  class input_port : virtual public port {
  };

  class cont_port : virtual public port {
  };

  class cont_output_port : public cont_port, public output_port {
  public:
    cont_output_port (setup* s, std::string id)
      : port (s, id) { }
    void map (data_map* dmap);
    void map (data_map* dmap, int max_buffered);
  };
  
  class cont_input_port : public cont_port, public input_port {
  public:
    cont_input_port (setup* s, std::string id)
      : port (s, id) { }
    void map (data_map* dmap, double delay = 0.0, bool interpolate = true);
    void map (data_map* dmap, int max_buffered, bool interpolate = true);
    void map (data_map* dmap,
	      double delay,
	      int max_buffered,
	      bool interpolate = true);
  };
  
  class event_port : virtual public port {
  };

  class event_output_port : public event_port, public output_port {
    event_router* router;
  public:
    event_output_port (setup* s, std::string id)
      : port (s, id) { }
    void map (index_map* indices);
    void map (index_map* indices, int max_buffered);
    void insert_event (double t, global_index id);
    void insert_event (double t, local_index id);
  };

  class event_input_port : public event_port, public input_port {
  public:
    event_input_port (setup* s, std::string id)
      : port (s, id) { }
    void map (index_map* indices,
	      event_handler_global_index* handle_event,
	      double acc_latency = 0.0);
    void map (index_map* indices,
	      event_handler_local_index* handle_event,
	      double acc_latency = 0.0);
    void map (index_map* indices,
	      event_handler_global_index* handle_event,
	      double acc_latency,
	      int max_buffered);
    void map (index_map* indices,
	      event_handler_local_index* handle_event,
	      double acc_latency,
	      int max_buffered);
    // Facilities to support the C interface
  public:
    event_handler_global_index_proxy*
    alloc_event_handler_global_index_proxy (void (*) (double, int));
    event_handler_local_index_proxy*
    alloc_event_handler_local_index_proxy (void (*) (double, int));
  private:
    event_handler_global_index_proxy c_event_handler_global_index;
    event_handler_local_index_proxy c_event_handler_local_index;
  };


  class message_port : virtual public port {
  };

  class message_output_port : public message_port, public output_port {
  public:
    void map ();
    void map (int max_buffered);
  };

  class message_input_port : public message_port, public input_port {
  public:
    void map (message_handler* handler, double acc_latency = 0.0);
    void map (message_handler* handler, double acc_latency, int max_buffered);
  };

}

#define MUSIC_PORT_HH
#endif

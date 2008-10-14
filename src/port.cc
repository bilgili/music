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

#include "music/port.hh"
#include "music/setup.hh"

namespace MUSIC {

  bool
  port::is_connected ()
  {
    if (_setup->launched_by_music ()) //*fixme*
      return true;
    else
      return false;
  }


  bool
  port::has_width ()
  {
    return _width != -1;
  }


  int
  port::width ()
  {
    return _width;
  }


  void
  cont_output_port::map (data_map* dmap)
  {
  }

  
  void
  cont_output_port::map (data_map* dmap, int max_buffered)
  {
  }

  
  void
  cont_input_port::map (data_map* dmap, double delay, bool interpolate)
  {
  }

  
  void
  cont_input_port::map (data_map* dmap,
			int max_buffered,
			bool interpolate)
  {
  }

  
  void
  cont_input_port::map (data_map* dmap,
			double delay,
			int max_buffered,
			bool interpolate)
  {
  }

  
  void
  event_output_port::map (index_map* indices)
  {
    int max_buffered = 2; //*fixme*
    map (indices, max_buffered);
  }

  
  void
  event_output_port::map (index_map* indices, int max_buffered)
  {
    event_output_connector* c
      = new event_output_connector (_setup->communicator ());
    router = new event_router (&c->buffer);
    _setup->add_output_connector (c);
  }


  void
  event_output_port::insert_event (double t, global_index id)
  {
    router->insert_event (t, id);
  }

  
  void
  event_output_port::insert_event (double t, local_index id)
  {
    router->insert_event (t, id);
  }

  
  void
  event_input_port::map (index_map* indices,
			 event_handler_global_index* handle_event,
			 double acc_latency)
  {
    int max_buffered = 2; //*fixme*
    map (indices, handle_event, acc_latency, max_buffered);
  }

  
  void
  event_input_port::map (index_map* indices,
			 event_handler_local_index* handle_event,
			 double acc_latency)
  {
    int max_buffered = 2; //*fixme*
    map (indices, handle_event, acc_latency, max_buffered);
  }

  
  void
  event_input_port::map (index_map* indices,
			 event_handler_global_index* handle_event,
			 double acc_latency,
			 int max_buffered)
  {
    event_input_connector* c
      = new event_input_connector (_setup->communicator (),
				   handle_event);
    _setup->add_input_connector (c);
  }

  
  void
  event_input_port::map (index_map* indices,
			 event_handler_local_index* handle_event,
			 double acc_latency,
			 int max_buffered)
  {
    event_input_connector* c
      = new event_input_connector (_setup->communicator (),
				   (event_handler_global_index*) handle_event);//*fixme*
    _setup->add_input_connector (c);
  }

  
  event_handler_global_index_proxy*
  event_input_port::alloc_event_handler_global_index_proxy (void (*eh) (double, int))
  {
    c_event_handler_global_index = event_handler_global_index_proxy (eh);
    return &c_event_handler_global_index;
  }

  
  event_handler_local_index_proxy*
  event_input_port::alloc_event_handler_local_index_proxy (void (*eh) (double, int))
  {
    c_event_handler_local_index = event_handler_local_index_proxy (eh);
    return &c_event_handler_local_index;
  }
  

  void
  message_output_port::map ()
  {
  }

  
  void
  message_output_port::map (int max_buffered)
  {
  }

  
  void
  message_input_port::map (message_handler* handler, double acc_latency)
  {
  }

  
  void
  message_input_port::map (message_handler* handler,
			   double acc_latency,
			   int max_buffered)
  {
  }

}

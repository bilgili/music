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
#include "music/error.hh"

namespace MUSIC {

  port::port (setup* s, std::string identifier)
    : _setup (s)
  {
    _connectivity_info = s->port_connectivity (identifier);
    _setup->add_port (this);
  }


  bool
  port::is_connected ()
  {
    return _connectivity_info != connectivity::NO_CONNECTIVITY;
  }


  void
  port::assert_output ()
  {
    if (!is_connected ())
      error ("attempt to map an unconnected port");
    else if (_connectivity_info->direction () != connectivity_info::OUTPUT)
      error ("output port connected as input");
  }


  void
  port::assert_input ()
  {
    if (!is_connected ())
      error ("attempt to map an unconnected port");
    else if (_connectivity_info->direction () != connectivity_info::INPUT)
      error ("input port connected as output");
  }


  bool
  port::has_width ()
  {
    return _connectivity_info->width () != connectivity_info::NO_WIDTH;
  }


  int
  port::width ()
  {
    int w = _connectivity_info->width ();
    if (w == connectivity_info::NO_WIDTH)
      error ("width requested for port with unspecified width");
    return w;
  }


  void
  output_redistribution_port::setup_cleanup ()
  {
    delete negotiator;
  }
  
  
  void
  input_redistribution_port::setup_cleanup ()
  {
    delete negotiator;
  }
  
  
  void
  cont_output_port::map (data_map* dmap)
  {
    assert_output ();
  }

  
  void
  cont_output_port::map (data_map* dmap, int max_buffered)
  {
    assert_output ();
  }

  
  void
  cont_input_port::map (data_map* dmap, double delay, bool interpolate)
  {
    assert_input ();
  }

  
  void
  cont_input_port::map (data_map* dmap,
			int max_buffered,
			bool interpolate)
  {
    assert_input ();
  }

  
  void
  cont_input_port::map (data_map* dmap,
			double delay,
			int max_buffered,
			bool interpolate)
  {
    assert_input ();
  }

  
  void
  event_output_port::map (index_map* indices, index::type type)
  {
    assert_output ();
    int max_buffered = 0;
    map_impl (indices, type, max_buffered);
  }

  
  void
  event_output_port::map (index_map* indices,
			  index::type type,
			  int max_buffered)
  {
    assert_output ();
    if (max_buffered <= 0)
      {
	error ("event_output_port::map: max_buffered should be a positive integer");
      }
    map_impl (indices, type, max_buffered);
  }

  
  void
  event_output_port::map_impl (index_map* indices,
			       index::type type,
			       int max_buffered)
  {
    MPI::Intracomm comm = _setup->communicator ();
    // Retrieve info about all remote connectors of this port
    port_connector_info port_connections
      = _connectivity_info->connections ();
    negotiator = new spatial_output_negotiator (indices, type);
    for (port_connector_info::iterator info = port_connections.begin ();
	 info != port_connections.end ();
	 ++info)
      // Create connector
      _setup->add_connector (new event_output_connector (*info,
							 negotiator,
							 max_buffered,
							 comm,
							 router));
  }


  void
  event_output_port::build_table ()
  {
    router.build_table ();
  }

  
  void
  event_output_port::insert_event (double t, global_index id)
  {
    router.insert_event (t, id);
  }

  
  void
  event_output_port::insert_event (double t, local_index id)
  {
    router.insert_event (t, id);
  }

  
  void
  event_input_port::map (index_map* indices,
			 event_handler_global_index* handle_event,
			 double acc_latency)
  {
    assert_input ();
    int max_buffered = 0;
    map_impl (indices,
	      index::GLOBAL,
	      event_handler_ptr (handle_event),
	      acc_latency,
	      max_buffered);
  }

  
  void
  event_input_port::map (index_map* indices,
			 event_handler_local_index* handle_event,
			 double acc_latency)
  {
    assert_input ();
    int max_buffered = 0;
    map_impl (indices,
	      index::LOCAL,
	      event_handler_ptr (handle_event),
	      acc_latency,
	      max_buffered);
  }

  
  void
  event_input_port::map (index_map* indices,
			 event_handler_global_index* handle_event,
			 double acc_latency,
			 int max_buffered)
  {
    assert_input ();
    if (max_buffered <= 0)
      {
	error ("event_input_port::map: max_buffered should be a positive integer");
      }
    map_impl (indices,
	      index::GLOBAL,
	      event_handler_ptr (handle_event),
	      acc_latency,
	      max_buffered);
  }

  
  void
  event_input_port::map (index_map* indices,
			 event_handler_local_index* handle_event,
			 double acc_latency,
			 int max_buffered)
  {
    assert_input ();
    if (max_buffered <= 0)
      {
	error ("event_input_port::map: max_buffered should be a positive integer");
      }
    map_impl (indices,
	      index::LOCAL,
	      event_handler_ptr (handle_event),
	      acc_latency,
	      max_buffered);
  }

  
  void
  event_input_port::map_impl (index_map* indices,
			      index::type type,
			      event_handler_ptr handle_event,
			      double acc_latency,
			      int max_buffered)
  {
    MPI::Intracomm comm = _setup->communicator ();
    // Retrieve info about all remote connectors of this port
    port_connector_info port_connections
      = _connectivity_info->connections ();
    port_connector_info::iterator info = port_connections.begin ();
    negotiator = new spatial_input_negotiator (indices, type);
    _setup->add_connector (new event_input_connector (*info,
						      negotiator,
						      handle_event,
						      type,
						      acc_latency,
						      max_buffered,
						      comm));
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
    assert_output ();
  }

  
  void
  message_output_port::map (int max_buffered)
  {
    assert_output ();
  }

  
  void
  message_input_port::map (message_handler* handler, double acc_latency)
  {
    assert_output ();
  }

  
  void
  message_input_port::map (message_handler* handler,
			   double acc_latency,
			   int max_buffered)
  {
    assert_output ();
  }

}

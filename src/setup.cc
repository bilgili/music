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

//#define MUSIC_DEBUG 1
#include "music/debug.hh"

#include <mpi.h>
#include "music/setup.hh"
#include "music/runtime.hh"
#include "music/parse.hh"

int debug_hang = 0;

static void
hang ()
{
  while (debug_hang)
    ;
}

namespace MUSIC {

  setup::setup (int& argc, char**& argv)
  {
    MPI::Init (argc, argv);
    MUSIC_LOG ("exiting MPI::Init");
    init (argc, argv);
  }

  
  setup::setup (int& argc, char**& argv, int required, int* provided)
  {
#ifdef HAVE_CXX_MPI_INIT_THREAD
    *provided = MPI::Init_thread (argc, argv, required);
#error hej
#else
    // Only C version provided in libmpich
    MPI_Init_thread (&argc, &argv, required, provided);
#endif
    init (argc, argv);
  }


  setup::~setup ()
  {
    for (std::vector<port*>::iterator i = _ports.begin ();
	 i != _ports.end ();
	 ++i)
      (*i)->setup_cleanup ();
  }
  

  bool
  setup::launched_by_music ()
  {
    return _config->launched_by_music ();
  }

  void
  setup::init (int& argc, char**& argv)
  {
    hang ();
    int my_rank = MPI::COMM_WORLD.Get_rank ();
    _config = new configuration ();
    if (launched_by_music ())
      {
	// launched by the music utility
	comm = MPI::COMM_WORLD.Split (_config->color (), my_rank);
	string binary;
	_config->lookup ("binary", &binary);
	string args;
	_config->lookup ("args", &args);
	argv = parse_args (binary, args, &argc);
      }
    else
      {
	// launched with mpirun
	comm = MPI::COMM_WORLD;
      }
  }


  MPI::Intracomm
  setup::communicator ()
  {
    return comm;
  }


  connectivity_info*
  setup::port_connectivity (const std::string local_name)
  {
    return _config->connectivity_map ()->info (local_name);
  }


  bool
  setup::is_connected (const std::string local_name)
  {
    return _config->connectivity_map ()->is_connected (local_name);
  }


  connectivity_info::port_direction
  setup::port_direction (const std::string local_name)
  {
    return _config->connectivity_map ()->direction (local_name);
  }


  int
  setup::port_width (const std::string local_name)
  {
    return _config->connectivity_map ()->width (local_name);
  }


  port_connector_info
  setup::port_connections (const std::string local_name)
  {
    return _config->connectivity_map ()->connections (local_name);
  }


  bool
  setup::config (string var, string* result)
  {
    return _config->lookup (var, result);
  }

  
  bool
  setup::config (string var, int* result)
  {
    return _config->lookup (var, result);
  }

  
  bool
  setup::config (string var, double* result)
  {
    return _config->lookup (var, result);
  }

  
  cont_input_port*
  setup::publish_cont_input (std::string identifier)
  {
    return new cont_input_port (this, identifier);
  }


  cont_output_port*
  setup::publish_cont_output (std::string identifier)
  {
    return new cont_output_port (this, identifier);
  }


  event_input_port*
  setup::publish_event_input (std::string identifier)
  {
    return new event_input_port (this, identifier);
  }


  event_output_port*
  setup::publish_event_output (std::string identifier)
  {
    return new event_output_port (this, identifier);
  }

  
  void setup::add_port (port* p)
  {
    _ports.push_back (p);
  }

  
  void setup::add_connector (connector* c)
  {
    _connectors.push_back (c);
  }

}

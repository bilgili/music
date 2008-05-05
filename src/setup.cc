/*
 *  This file is part of MUSIC.
 *  Copyright (C) 2007, 2008 CSC, KTH
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

#include <mpi.h>
#include "music/setup.hh"
#include "music/runtime.hh"
#include "music/parse.hh"

namespace MUSIC {

  setup::setup (int& argc, char**& argv)
  {
    MPI::Init (argc, argv);
    int my_rank = MPI::COMM_WORLD.Get_rank ();
    _config = new configuration ();
    if (_config->launched_by_music ())
      {
	// launched by the music utility
	my_communicator = MPI::COMM_WORLD.Split (_config->color (), my_rank);
	string binary;
	_config->lookup ("binary", &binary);
	string args;
	_config->lookup ("args", &args);
	argv = parse_args (binary, args, &argc);
      }
    else
      {
	// launched with mpirun
	my_communicator = MPI::COMM_WORLD;
      }
  }


  MPI::Intracomm
  setup::communicator ()
  {
    return my_communicator;
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

  
  bool
  setup::is_port (string identifier)
  {
  }

  
  int
  setup::port_size (string identifier)
  {
  }

  
  cont_input_port*
  setup::publish_input (std::string identifier, cont_data* map)
  {
  }


  cont_output_port*
  setup::publish_output (std::string identifier, cont_data* map)
  {
  }


  event_input_port*
  setup::publish_input (std::string identifier, event_data* map)
  {
    return new event_input_port (map);
  }


  event_output_port*
  setup::publish_output (std::string identifier, event_data* map)
  {
    return new event_output_port (map);
  }


  runtime*
  setup::done ()
  {
    return new runtime (my_communicator);
  }

}

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

  void
  setup::init (int& argc, char**& argv)
  {
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

  
  cont_input_port*
  setup::publish_cont_input (std::string identifier)
  {
    return new cont_input_port ();
  }


  cont_output_port*
  setup::publish_cont_output (std::string identifier)
  {
    return new cont_output_port ();
  }


  event_input_port*
  setup::publish_event_input (std::string identifier)
  {
    return new event_input_port ();
  }


  event_output_port*
  setup::publish_event_output (std::string identifier)
  {
    return new event_output_port ();
  }


}

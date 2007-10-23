/*
 *  This file is part of MUSIC.
 *  Copyright (C) 2007 CSC, KTH
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

namespace MUSIC {

  setup::setup (int color, int& argc, char**& argv)
  {
    int my_rank;

    MPI::Init (argc, argv);
    my_rank = MPI::COMM_WORLD.Get_rank ();
    myCommunicator = MPI::COMM_WORLD.Split (color, my_rank);
  }


  MPI::Intracomm
  setup::communicator ()
  {
    return myCommunicator;
  }


  string
  config_string (string var)
  {
  }

  int
  config_int (string var)
  {
  }

  double
  config_double (string var)
  {
  }

  bool
  is_port (string identifier)
  {
  }

  int
  port_size (string identifier)
  {
  }

  void
  setup::publish (data_map* map, std::string identifier)
  {
  }


  runtime*
  setup::done ()
  {
    return new runtime (myCommunicator);
  }

}

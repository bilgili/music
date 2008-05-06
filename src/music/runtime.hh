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

#ifndef MUSIC_RUNTIME_HH

#include <mpi.h>
#include <vector>

#include "music/setup.hh"
#include "music/port.hh"
#include "music/clock.hh"
#include "music/connector.hh"

namespace MUSIC {

  class runtime {
  private:
    MPI::Intracomm myCommunicator;
    clock local_time;
    std::vector<input_port>* input_ports;
    std::vector<output_port>* output_ports;
    std::vector<connector*>* schedule;
  
  public:
    runtime (setup* s, double h);
    
    MPI::Intracomm
    communicator ();

    void
    finalize ();

    void
    tick ();

    double
    time ();
  };

}

#define MUSIC_RUNTIME_HH
#endif

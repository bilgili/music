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

#ifndef MUSIC_SETUP_HH

#include <mpi.h>

#include <string>

#include <music/runtime.hh>

#include <music/index_map.hh>
#include <music/linear_index.hh>
#include <music/data_map.hh>
#include <music/array_data.hh>


namespace MUSIC {
  
  class setup {
  private:
    MPI_Comm myCommunicator;

  public:
    setup (int color, int* argc, char** argv[]);

    MPI_Comm
    communicator ();

    void
    publish (data_map* map, std::string identifier);

    runtime*
    done ();
  };
  
}

#define MUSIC_SETUP_HH
#endif

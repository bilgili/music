/*
 *  This file is part of MUSIC.
 *  Copyright (C) 2007, 2008, 2009 CSC, KTH
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

  class Runtime {
  private:
    MPI::Intracomm comm;
    Clock localTime;
    std::vector<Connector*>* connectors;
    std::vector<OutputSubconnector*> outputSubconnectors;
    std::vector<InputSubconnector*> inputSubconnectors;
    std::vector<Subconnector*> schedule;

    void spatialNegotiation (Setup* s);
    void buildSchedule (int localRank);
    void buildTables (Setup* s);
    void temporalNegotiation (Setup* s, Clock& localTime);

    int rank;
    Clock* sendClock;
    Clock* receiveClock;
  
  public:
    Runtime (Setup* s, double h);
    ~Runtime ();
    
    MPI::Intracomm communicator ();

    void finalize ();

    void tick ();

    double time ();
  };

}

#define MUSIC_RUNTIME_HH
#endif

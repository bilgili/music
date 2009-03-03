/*
 *  This file is part of MUSIC.
 *  Copyright (C) 2007, 2008, 2009 INCF
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

  /*
   * This is the Runtime object in the MUSIC API
   *
   * It is documented in section 4.4 of the MUSIC manual
   */
  
  class Runtime {
  private:
    MPI::Intracomm comm;
    Clock localTime;
    std::vector<TickingPort*> tickingPorts;
    std::vector<Connector*>* connectors;
    std::vector<OutputSubconnector*> outputSubconnectors;
    std::vector<InputSubconnector*> inputSubconnectors;
    std::vector<Subconnector*> schedule;
    std::vector<PostCommunicationConnector*> postCommunication;

    void takeTickingPorts (Setup* s);
    void connectToPeers ();
    void specializeConnectors ();
    void spatialNegotiation ();
    void buildSchedule (int localRank);
    void takePostCommunicators ();
    void buildTables (Setup* s);
    void temporalNegotiation (Setup* s);
    void initialize ();

    int rank;
  
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

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

#ifndef MUSIC_SETUP_HH

#include <mpi.h>

#include <string>
#include <vector>

#include <music/port.hh>

#include <music/index_map.hh>
#include <music/linear_index.hh>
#include <music/cont_data.hh>
#include <music/configuration.hh>
#include <music/connector.hh>

using std::string;

namespace MUSIC {
  
  class setup {
  private:
    configuration* _config;
    MPI::Intracomm my_communicator;
    MPI::Intracomm _global_comm_dup;
    std::vector<input_port*>* input_ports;
    std::vector<output_port*>* output_ports;
    std::vector<output_connector*>* _output_connectors;
    std::vector<input_connector*>* _input_connectors;

  public:
    setup (int& argc, char**& argv);

    setup (int& argc, char**& argv, int required, int* provided);

    void init (int& argc, char**& argv);

    MPI::Intracomm communicator ();

    MPI::Intracomm global_comm_dup () { return _global_comm_dup; }
    
    bool config (string var, string* result);

    bool config (string var, int* result);

    bool config (string var, double* result);

    cont_input_port* publish_cont_input (string identifier);

    cont_output_port* publish_cont_output (string identifier);

    event_input_port* publish_event_input (string identifier);

    event_output_port* publish_event_output (string identifier);

    message_input_port* publish_message_input (string identifier);

    message_output_port* publish_message_output (string identifier);

    std::vector<input_connector*>* input_connectors ()
    {
      return _input_connectors;
    }

    void add_input_connector (input_connector* c);
    
    std::vector<output_connector*>* output_connectors ()
    {
      return _output_connectors;
    }
    
    void add_output_connector (output_connector* c);
    
  };
  
}

#define MUSIC_SETUP_HH
#endif

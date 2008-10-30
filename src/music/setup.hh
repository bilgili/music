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
    MPI::Intracomm comm;
    std::vector<port*> _ports;
    std::vector<connector*> _connectors;

  public:
    setup (int& argc, char**& argv);

    setup (int& argc, char**& argv, int required, int* provided);

    virtual ~setup ();

    bool launched_by_music ();

    void init (int& argc, char**& argv);

    MPI::Intracomm communicator ();

    connectivity_info* port_connectivity (const std::string local_name);

    //*fixme* unused
    bool is_connected (const std::string local_name);

    connectivity_info::port_direction
    port_direction (const std::string local_name);

    int port_width (const std::string local_name);

    port_connector_info port_connections (const std::string local_name);

    bool config (string var, string* result);

    bool config (string var, int* result);

    bool config (string var, double* result);

    cont_input_port* publish_cont_input (string identifier);

    cont_output_port* publish_cont_output (string identifier);

    event_input_port* publish_event_input (string identifier);

    event_output_port* publish_event_output (string identifier);

    message_input_port* publish_message_input (string identifier);

    message_output_port* publish_message_output (string identifier);

    std::vector<port*>* ports ()
    {
      return &_ports;
    }    

    void add_port (port* p);
    
    std::vector<connector*>* connectors ()
    {
      return &_connectors;
    }
    
    void add_connector (connector* c);
    
  };
  
}

#define MUSIC_SETUP_HH
#endif

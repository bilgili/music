/*
 *  This file is part of MUSIC.
 *  Copyright (C) 2008 INCF
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

#ifndef MUSIC_CONNECTIVITY_HH

#include <sstream>
#include <vector>
#include <map>

namespace MUSIC {

  class connector_info {
    std::string _rec_app;
    std::string _rec_port;
    int _remote_leader;
    int _n_proc;
  public:
    connector_info () { }
    connector_info (std::string rec_app,
		    std::string rec_name,
		    int r_leader,
		    int n_proc)
      : _rec_app (rec_app),
	_rec_port (rec_name),
	_remote_leader (r_leader),
	_n_proc (n_proc)
    { }
    std::string receiver_app_name () const { return _rec_app; }
    std::string receiver_port_name () const { return _rec_port; }
    int remote_leader () const { return _remote_leader; }
    int n_processes () const { return _n_proc; } //*fixme* "remote" in name
  };


  typedef std::vector<connector_info> port_connector_info;


  class connectivity_info {
  public:
    enum port_direction { OUTPUT, INPUT };
    static const int NO_WIDTH = -1;
  private:
    port_direction _dir;
    int _width;
    port_connector_info _port_connections;
  public:
    connectivity_info (port_direction dir, int width)
      : _dir (dir), _width (width) { }
    port_direction direction () { return _dir; }
    int width () { return _width; } // NO_WIDTH if no width specified
    port_connector_info connections () { return _port_connections; }
    void add_connection (std::string rec_app,
			 std::string rec_name,
			 int r_leader,
			 int n_proc);
  };

  
  class connectivity {
    std::vector<connectivity_info> _connections;
    std::map<std::string, int> connectivity_map;
    void read (std::istringstream& in);
  public:
    connectivity () { }
    connectivity (std::istringstream& in);
    static const int NO_CONNECTIVITY = 0;
    void add (std::string local_port,
	      connectivity_info::port_direction dir,
	      int width,
	      std::string rec_app,
	      std::string rec_port,
	      int remote_leader,
	      int remote_n_proc);
    connectivity_info* info (std::string port_name);
    //*fixme* not used
    bool is_connected (std::string port_name);
    connectivity_info::port_direction direction (std::string port_name);
    int width (std::string port_name);
    port_connector_info connections (std::string port_name);
    void write (std::ostringstream& out);
  };

}

#define MUSIC_CONNECTIVITY_HH
#endif

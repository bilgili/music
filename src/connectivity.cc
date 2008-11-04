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

//#define MUSIC_DEBUG 1
#include "music/debug.hh"

#include "music/connectivity.hh"
#include "music/ioutils.hh"
#include "music/error.hh"

namespace MUSIC {

  void
  connectivity_info::add_connection (std::string rec_app,
				     std::string rec_name,
				     int r_leader,
				     int n_proc)
  {
    _port_connections.push_back (connector_info (rec_app,
						 rec_name,
						 r_leader,
						 n_proc));
  }


  connectivity::connectivity (std::istringstream& in)
  {
    read (in);
  }
  

  void
  connectivity::add (std::string local_port,
		     connectivity_info::port_direction dir,
		     int width,
		     std::string rec_app,
		     std::string rec_port,
		     int remote_leader,
		     int remote_n_proc)
  {
    std::map<std::string, int>::iterator cmap_info
      = connectivity_map.find (local_port);
    connectivity_info* info;
    if (cmap_info == connectivity_map.end ())
      {
	MUSIC_LOG ("creating new entry for " << local_port);
	int index = _connections.size ();
	_connections.push_back (connectivity_info (dir, width));
	info = &_connections.back ();
	MUSIC_LOG ("ci = " << info);
	connectivity_map.insert (std::make_pair (local_port, index));
      }
    else
      {
	MUSIC_LOG ("found old entry for " << local_port);
	info = &_connections[cmap_info->second];
	if (info->direction () != dir)
	  error ("port " + local_port + " used both as output and input");
      }
    info->add_connection (rec_app, rec_port, remote_leader, remote_n_proc);
  }


  connectivity_info*
  connectivity::info (std::string port_name)
  {
    std::map<std::string, int>::iterator info
      = connectivity_map.find (port_name);
    if (info == connectivity_map.end ())
      return NO_CONNECTIVITY;
    else
      return &_connections[info->second];
  }


  bool
  connectivity::is_connected (std::string port_name)
  {
    return connectivity_map.find (port_name) != connectivity_map.end ();
  }


  connectivity_info::port_direction
  connectivity::direction (std::string port_name)
  {
    return _connections[connectivity_map[port_name]].direction ();
  }

  
  int
  connectivity::width (std::string port_name)
  {
    return _connections[connectivity_map[port_name]].width ();
  }

  
  port_connector_info
  connectivity::connections (std::string port_name)
  {
    return _connections[connectivity_map[port_name]].connections ();
  }


  void
  connectivity::write (std::ostringstream& out)
  {
    out << connectivity_map.size ();
    std::map<std::string, int>::iterator i;
    for (i = connectivity_map.begin ();
	 i != connectivity_map.end ();
	 ++i)
      {
	out << ':' << i->first << ':';
	connectivity_info* ci = &_connections[i->second];
	out << ci->direction () << ':' << ci->width () << ':';
	port_connector_info conns = ci->connections ();
	out << conns.size ();
	port_connector_info::iterator c;
	for (c = conns.begin (); c != conns.end (); ++c)
	  {
	    out << ':' << c->receiver_app_name ();
	    out << ':' << c->receiver_port_name ();
	    out << ':' << c->remote_leader ();
	    out << ':' << c->n_processes ();
	  }
      }
  }
  

  void
  connectivity::read (std::istringstream& in)
  {
    int n_ports;
    in >> n_ports;
    for (int i = 0; i < n_ports; ++i)
      {
	in.ignore ();
	std::string port_name = ioutils::read (in);
	in.ignore ();
	int dir;
	in >> dir;
	connectivity_info::port_direction pdir
	  = static_cast<connectivity_info::port_direction> (dir);
	in.ignore ();
	int width;
	in >> width;
	in.ignore ();
	int n_connections;
	in >> n_connections;
	for (int i = 0; i < n_connections; ++i)
	  {
	    in.ignore ();
	    std::string rec_app = ioutils::read (in);
	    in.ignore ();
	    std::string rec_port = ioutils::read (in);
	    in.ignore ();
	    int r_leader;
	    in >> r_leader;
	    in.ignore ();
	    int n_proc;
	    in >> n_proc;
	    add (port_name, pdir, width, rec_app, rec_port, r_leader, n_proc);
	  }
      }
  }
  
}

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
  ConnectivityInfo::addConnection (std::string recApp,
				     std::string recName,
				     int rLeader,
				     int nProc)
  {
    _portConnections.push_back (ConnectorInfo (recApp,
						 recName,
						 rLeader,
						 nProc));
  }


  Connectivity::Connectivity (std::istringstream& in)
  {
    read (in);
  }
  

  void
  Connectivity::add (std::string localPort,
		     ConnectivityInfo::PortDirection dir,
		     int width,
		     std::string recApp,
		     std::string recPort,
		     int remoteLeader,
		     int remoteNProc)
  {
    std::map<std::string, int>::iterator cmapInfo
      = connectivityMap.find (localPort);
    ConnectivityInfo* info;
    if (cmapInfo == connectivityMap.end ())
      {
	MUSIC_LOG ("creating new entry for " << localPort);
	int index = _connections.size ();
	_connections.push_back (ConnectivityInfo (dir, width));
	info = &_connections.back ();
	MUSIC_LOG ("ci = " << info);
	connectivityMap.insert (std::make_pair (localPort, index));
      }
    else
      {
	MUSIC_LOG ("found old entry for " << localPort);
	info = &_connections[cmapInfo->second];
	if (info->direction () != dir)
	  error ("port " + localPort + " used both as output and input");
      }
    info->addConnection (recApp, recPort, remoteLeader, remoteNProc);
  }


  ConnectivityInfo*
  Connectivity::info (std::string portName)
  {
    std::map<std::string, int>::iterator info
      = connectivityMap.find (portName);
    if (info == connectivityMap.end ())
      return NO_CONNECTIVITY;
    else
      return &_connections[info->second];
  }


  bool
  Connectivity::isConnected (std::string portName)
  {
    return connectivityMap.find (portName) != connectivityMap.end ();
  }


  ConnectivityInfo::PortDirection
  Connectivity::direction (std::string portName)
  {
    return _connections[connectivityMap[portName]].direction ();
  }

  
  int
  Connectivity::width (std::string portName)
  {
    return _connections[connectivityMap[portName]].width ();
  }

  
  PortConnectorInfo
  Connectivity::connections (std::string portName)
  {
    return _connections[connectivityMap[portName]].connections ();
  }


  void
  Connectivity::write (std::ostringstream& out)
  {
    out << connectivityMap.size ();
    std::map<std::string, int>::iterator i;
    for (i = connectivityMap.begin ();
	 i != connectivityMap.end ();
	 ++i)
      {
	out << ':' << i->first << ':';
	ConnectivityInfo* ci = &_connections[i->second];
	out << ci->direction () << ':' << ci->width () << ':';
	PortConnectorInfo conns = ci->connections ();
	out << conns.size ();
	PortConnectorInfo::iterator c;
	for (c = conns.begin (); c != conns.end (); ++c)
	  {
	    out << ':' << c->receiverAppName ();
	    out << ':' << c->receiverPortName ();
	    out << ':' << c->remoteLeader ();
	    out << ':' << c->nProcesses ();
	  }
      }
  }
  

  void
  Connectivity::read (std::istringstream& in)
  {
    int nPorts;
    in >> nPorts;
    for (int i = 0; i < nPorts; ++i)
      {
	in.ignore ();
	std::string portName = IOUtils::read (in);
	in.ignore ();
	int dir;
	in >> dir;
	ConnectivityInfo::PortDirection pdir
	  = static_cast<ConnectivityInfo::PortDirection> (dir);
	in.ignore ();
	int width;
	in >> width;
	in.ignore ();
	int nConnections;
	in >> nConnections;
	for (int i = 0; i < nConnections; ++i)
	  {
	    in.ignore ();
	    std::string recApp = IOUtils::read (in);
	    in.ignore ();
	    std::string recPort = IOUtils::read (in);
	    in.ignore ();
	    int rLeader;
	    in >> rLeader;
	    in.ignore ();
	    int nProc;
	    in >> nProc;
	    add (portName, pdir, width, recApp, recPort, rLeader, nProc);
	  }
      }
  }
  
}

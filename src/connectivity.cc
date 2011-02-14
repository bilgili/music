/*
 *  This file is part of MUSIC.
 *  Copyright (C) 2008, 2009 INCF
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
				   int recCode,
				   int rLeader,
				   int nProc)
  {
    portConnections_.push_back (ConnectorInfo (recApp,
					       recName,
					       recCode,
					       rLeader,
					       nProc,
					       width_));
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
		     int recPortCode,
		     int remoteLeader,
		     int remoteNProc)
  {
    std::map<std::string, int>::iterator cmapInfo
      = connectivityMap.find (localPort);
    ConnectivityInfo* info;
    if (cmapInfo == connectivityMap.end ())
      {
	MUSIC_LOG ("creating new entry for " << localPort);
	int index = connections_.size ();
	connections_.push_back (ConnectivityInfo (localPort, dir, width));
	info = &connections_.back ();
	MUSIC_LOG ("ci = " << info);
	connectivityMap.insert (std::make_pair (localPort, index));
      }
    else
      {
	MUSIC_LOG ("found old entry for " << localPort);
	info = &connections_[cmapInfo->second];
	if (info->direction () != dir)
	  error ("port " + localPort + " used both as output and input");
      }
    info->addConnection (recApp,
			 recPort,
			 recPortCode,
			 remoteLeader,
			 remoteNProc);
  }


  ConnectivityInfo*
  Connectivity::info (std::string portName)
  {
    std::map<std::string, int>::iterator info
      = connectivityMap.find (portName);
    if (info == connectivityMap.end ())
      return NO_CONNECTIVITY;
    else
      return &connections_[info->second];
  }


  bool
  Connectivity::isConnected (std::string portName)
  {
    return connectivityMap.find (portName) != connectivityMap.end ();
  }


  ConnectivityInfo::PortDirection
  Connectivity::direction (std::string portName)
  {
    return connections_[connectivityMap[portName]].direction ();
  }

  
  int
  Connectivity::width (std::string portName)
  {
    return connections_[connectivityMap[portName]].width ();
  }

  
  PortConnectorInfo
  Connectivity::connections (std::string portName)
  {
    return connections_[connectivityMap[portName]].connections ();
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
	ConnectivityInfo* ci = &connections_[i->second];
	out << ci->direction () << ':' << ci->width () << ':';
	PortConnectorInfo conns = ci->connections ();
	out << conns.size ();
	PortConnectorInfo::iterator c;
	for (c = conns.begin (); c != conns.end (); ++c)
	  {
	    out << ':' << c->receiverAppName ();
	    out << ':' << c->receiverPortName ();
	    out << ':' << c->receiverPortCode ();
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
	    int recPortCode;
	    in >> recPortCode;
	    in.ignore ();
	    int rLeader;
	    in >> rLeader;
	    in.ignore ();
	    int nProc;
	    in >> nProc;
	    add (portName,
		 pdir,
		 width,
		 recApp,
		 recPort,
		 recPortCode,
		 rLeader,
		 nProc);
	    MUSIC_LOG ("add (portName = " << portName
		       << ", pdir = " << pdir
		       << ", width = " << width
		       << ", recApp = " << recApp
		       << ", recPort = " << recPort
		       << ", rLeader = " << rLeader
		       << ", nProc = " << nProc
		       << ")");
	  }
      }
  }
  
}

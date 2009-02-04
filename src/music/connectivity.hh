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

#ifndef MUSIC_CONNECTIVITY_HH

#include <sstream>
#include <vector>
#include <map>

namespace MUSIC {

  class ConnectorInfo {
    std::string _recApp;
    std::string _recPort;
    int _recCode;
    int _remoteLeader;
    int _nProc;
  public:
    ConnectorInfo () { }
    ConnectorInfo (std::string recApp,
		   std::string recName,
		   int recCode,
		   int rLeader,
		   int nProc)
      : _recApp (recApp),
	_recPort (recName),
	_recCode (recCode),
	_remoteLeader (rLeader),
	_nProc (nProc)
    { }
    std::string receiverAppName () const { return _recApp; }
    std::string receiverPortName () const { return _recPort; }
    int receiverPortCode () const { return _recCode; }
    int remoteLeader () const { return _remoteLeader; }
    int nProcesses () const { return _nProc; } //*fixme* "remote" in name
  };


  typedef std::vector<ConnectorInfo> PortConnectorInfo;


  class ConnectivityInfo {
  public:
    enum PortDirection { OUTPUT, INPUT };
    static const int NO_WIDTH = -1;
  private:
    PortDirection _dir;
    int _width;
    PortConnectorInfo _portConnections;
  public:
    ConnectivityInfo (PortDirection dir, int width)
      : _dir (dir), _width (width) { }
    PortDirection direction () { return _dir; }
    int width () { return _width; } // NO_WIDTH if no width specified
    PortConnectorInfo connections () { return _portConnections; }
    void addConnection (std::string recApp,
			std::string recName,
			int recCode,
			int rLeader,
			int nProc);
  };

  
  class Connectivity {
    std::vector<ConnectivityInfo> _connections;
    std::map<std::string, int> connectivityMap;
    void read (std::istringstream& in);
  public:
    Connectivity () { }
    Connectivity (std::istringstream& in);
    static const int NO_CONNECTIVITY = 0;
    void add (std::string localPort,
	      ConnectivityInfo::PortDirection dir,
	      int width,
	      std::string recApp,
	      std::string recPort,
	      int recPortCode,
	      int remoteLeader,
	      int remoteNProc);
    ConnectivityInfo* info (std::string portName);
    //*fixme* not used
    bool isConnected (std::string portName);
    ConnectivityInfo::PortDirection direction (std::string portName);
    int width (std::string portName);
    PortConnectorInfo connections (std::string portName);
    void write (std::ostringstream& out);
  };

}

#define MUSIC_CONNECTIVITY_HH
#endif

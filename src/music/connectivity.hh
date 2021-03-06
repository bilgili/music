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
    std::string recApp_;
    std::string recPort_;
    int recCode_;
    int remoteLeader_;
    int nProc_;
  public:
    ConnectorInfo () { }
    ConnectorInfo (std::string recApp,
		   std::string recName,
		   int recCode,
		   int rLeader,
		   int nProc)
      : recApp_ (recApp),
	recPort_ (recName),
	recCode_ (recCode),
	remoteLeader_ (rLeader),
	nProc_ (nProc)
    { }
    std::string receiverAppName () const { return recApp_; }
    std::string receiverPortName () const { return recPort_; }
    int receiverPortCode () const { return recCode_; }
    int remoteLeader () const { return remoteLeader_; }
    // NOTE: nProcesses should have "remote" in name
    int nProcesses () const { return nProc_; }
  };


  typedef std::vector<ConnectorInfo> PortConnectorInfo;


  class ConnectivityInfo {
  public:
    enum PortDirection { OUTPUT, INPUT };
    static const int NO_WIDTH = -1;
  private:
    std::string portName_;
    PortDirection dir_;
    int width_;
    PortConnectorInfo portConnections_;
  public:
    ConnectivityInfo (std::string name, PortDirection dir, int portWidth)
      : portName_ (name), dir_ (dir), width_ (portWidth) { }
    std::string portName () { return portName_; }
    PortDirection direction () { return dir_; }
    int width () { return width_; } // NO_WIDTH if no width specified
    PortConnectorInfo connections () { return portConnections_; }
    void addConnection (std::string recApp,
			std::string recName,
			int recCode,
			int rLeader,
			int nProc);
  };

  
  class Connectivity {
    std::vector<ConnectivityInfo> connections_;
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
    bool isConnected (std::string portName);
    ConnectivityInfo::PortDirection direction (std::string portName);
    int width (std::string portName);
    PortConnectorInfo connections (std::string portName);
    void write (std::ostringstream& out);
  };

}

#define MUSIC_CONNECTIVITY_HH
#endif

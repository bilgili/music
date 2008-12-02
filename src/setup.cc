/*
 *  This file is part of MUSIC.
 *  Copyright (C) 2007, 2008 INCF
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

#include <mpi.h>
#include "music/setup.hh"
#include "music/runtime.hh"
#include "music/parse.hh"

int debugHang = 0;

static void
hang ()
{
  while (debugHang)
    ;
}

namespace MUSIC {

  Setup::Setup (int& argc, char**& argv)
  {
    MPI::Init (argc, argv);
    MUSIC_LOG ("exiting MPI::Init");
    init (argc, argv);
  }

  
  Setup::Setup (int& argc, char**& argv, int required, int* provided)
  {
#ifdef HAVE_CXX_MPI_INIT_THREAD
    *provided = MPI::Init_thread (argc, argv, required);
#error hej
#else
    // Only C version provided in libmpich
    MPI_Init_thread (&argc, &argv, required, provided);
#endif
    init (argc, argv);
  }


  Setup::~Setup ()
  {
    for (std::vector<Port*>::iterator i = _ports.begin ();
	 i != _ports.end ();
	 ++i)
      (*i)->setupCleanup ();
  }
  

  bool
  Setup::launchedByMusic ()
  {
    return _config->launchedByMusic ();
  }

  void
  Setup::init (int& argc, char**& argv)
  {
    hang ();
    int myRank = MPI::COMM_WORLD.Get_rank ();
    _config = new Configuration ();
    if (launchedByMusic ())
      {
	// launched by the music utility
	comm = MPI::COMM_WORLD.Split (_config->color (), myRank);
	string binary;
	_config->lookup ("binary", &binary);
	string args;
	_config->lookup ("args", &args);
	argv = parseArgs (binary, args, &argc);
      }
    else
      {
	// launched with mpirun
	comm = MPI::COMM_WORLD;
      }
  }


  MPI::Intracomm
  Setup::communicator ()
  {
    return comm;
  }


  ConnectivityInfo*
  Setup::portConnectivity (const std::string localName)
  {
    return _config->connectivityMap ()->info (localName);
  }


  bool
  Setup::isConnected (const std::string localName)
  {
    return _config->connectivityMap ()->isConnected (localName);
  }


  ConnectivityInfo::PortDirection
  Setup::portDirection (const std::string localName)
  {
    return _config->connectivityMap ()->direction (localName);
  }


  int
  Setup::portWidth (const std::string localName)
  {
    return _config->connectivityMap ()->width (localName);
  }


  PortConnectorInfo
  Setup::portConnections (const std::string localName)
  {
    return _config->connectivityMap ()->connections (localName);
  }


  bool
  Setup::config (string var, string* result)
  {
    return _config->lookup (var, result);
  }

  
  bool
  Setup::config (string var, int* result)
  {
    return _config->lookup (var, result);
  }

  
  bool
  Setup::config (string var, double* result)
  {
    return _config->lookup (var, result);
  }

  
  ContInputPort*
  Setup::publishContInput (std::string identifier)
  {
    return new ContInputPort (this, identifier);
  }


  ContOutputPort*
  Setup::publishContOutput (std::string identifier)
  {
    return new ContOutputPort (this, identifier);
  }


  EventInputPort*
  Setup::publishEventInput (std::string identifier)
  {
    return new EventInputPort (this, identifier);
  }


  EventOutputPort*
  Setup::publishEventOutput (std::string identifier)
  {
    return new EventOutputPort (this, identifier);
  }

  
  void Setup::addPort (Port* p)
  {
    _ports.push_back (p);
  }

  
  void Setup::addConnector (Connector* c)
  {
    _connectors.push_back (c);
  }

}

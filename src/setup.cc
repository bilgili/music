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
#else
    // Only C version provided in libmpich
    MPI_Init_thread (&argc, &argv, required, provided);
#endif
    init (argc, argv);
  }


  Setup::~Setup ()
  {
    for (std::vector<Port*>::iterator i = ports_.begin ();
	 i != ports_.end ();
	 ++i)
      (*i)->setupCleanup ();
  }
  

  bool
  Setup::launchedByMusic ()
  {
    return config_->launchedByMusic ();
  }

  void
  Setup::init (int& argc, char**& argv)
  {
    hang ();
    int myRank = MPI::COMM_WORLD.Get_rank ();
    config_ = new Configuration ();
    connectors_ = new std::vector<Connector*>; // destoyed by runtime
    if (launchedByMusic ())
      {
	// launched by the music utility
	comm = MPI::COMM_WORLD.Split (config_->color (), myRank);
	MUSIC_LOG (comm.Get_rank () << ": " << comm);
	if (!config ("timebase", &timebase_))
	  timebase_ = 1e-9;		// default timebase
	string binary;
	config_->lookup ("binary", &binary);
	string args;
	config_->lookup ("args", &args);
	argv = parseArgs (binary, args, &argc);
	temporalNegotiator_ = new TemporalNegotiator (this);
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
    return config_->connectivityMap ()->info (localName);
  }


  ApplicationMap*
  Setup::applicationMap ()
  {
    return config_->applications ();
  }


  bool
  Setup::isConnected (const std::string localName)
  {
    return config_->connectivityMap ()->isConnected (localName);
  }


  ConnectivityInfo::PortDirection
  Setup::portDirection (const std::string localName)
  {
    return config_->connectivityMap ()->direction (localName);
  }


  int
  Setup::portWidth (const std::string localName)
  {
    return config_->connectivityMap ()->width (localName);
  }


  PortConnectorInfo
  Setup::portConnections (const std::string localName)
  {
    return config_->connectivityMap ()->connections (localName);
  }


  bool
  Setup::config (string var, string* result)
  {
    return config_->lookup (var, result);
  }

  
  bool
  Setup::config (string var, int* result)
  {
    return config_->lookup (var, result);
  }

  
  bool
  Setup::config (string var, double* result)
  {
    return config_->lookup (var, result);
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
    ports_.push_back (p);
  }

  
  void Setup::addConnector (Connector* c)
  {
    connectors_->push_back (c);
  }

}

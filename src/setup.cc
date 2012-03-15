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

#include "config.h"

#include "music/setup.hh"
#include "music/runtime.hh"
#include "music/parse.hh"
#include "music/error.hh"

namespace MUSIC {

  bool Setup::isInstantiated_ = false;
  static std::string err_MPI_Init = "MPI_Init was called before the Setup constructor";

  Setup::Setup (int& argc, char**& argv)
    : argc_ (argc), argv_ (argv)
  {
    checkInstantiatedOnce (isInstantiated_, "Setup");
    if (MPI::Is_initialized ())
      errorRank (err_MPI_Init);
    MPI::Init (argc, argv);
    init (argc, argv);
  }

  
  Setup::Setup (int& argc, char**& argv, int required, int* provided)
    : argc_ (argc), argv_ (argv)
  {
    checkInstantiatedOnce (isInstantiated_, "Setup");
    if (MPI::Is_initialized ())
      errorRank (err_MPI_Init);
#ifdef HAVE_CXX_MPI_INIT_THREAD
    *provided = MPI::Init_thread (argc, argv, required);
#else
    // Only C version provided in libmpich
    MPI_Init_thread (&argc, &argv, required, provided);
#endif
    init (argc, argv);
  }


  void
  Setup::init (int& argc, char**& argv)
  {
    int myRank = MPI::COMM_WORLD.Get_rank ();
    config_ = new Configuration ();
    connections_ = new std::vector<Connection*>; // destroyed by runtime
    if (launchedByMusic ())
      {
	// launched by the music utility
	if (!config_->postponeSetup ())
	  fullInit ();
	comm = MPI::COMM_WORLD.Split (config_->color (), myRank);
      }
    else
      {
	// launched with mpirun
	comm = MPI::COMM_WORLD;
	timebase_ = MUSIC_DEFAULT_TIMEBASE;
      }
  }


  void
  Setup::maybePostponedSetup ()
  {
    if (config_->postponeSetup ())
      {
	delete config_;
	config_ = new Configuration ();
	fullInit ();
      }
  }


  void
  Setup::errorChecks ()
  {
    ApplicationMap* apps = applicationMap ();
    int nRequestedProc = apps->nProcesses ();
    int nMPIProc = MPI::COMM_WORLD.Get_size ();
    if (nMPIProc != nRequestedProc)
      {
	std::ostringstream msg;
	msg << "configuration file specifies " << nRequestedProc
	    << " MPI processes but MUSIC was given " << nMPIProc
	    << std::endl;
	error0 (msg.str ());
      }
  }

  void
  Setup::fullInit ()
  {
    errorChecks ();
    if (!config ("timebase", &timebase_))
      timebase_ = MUSIC_DEFAULT_TIMEBASE;	       // default timebase
    string binary;
    config_->lookup ("binary", &binary);
    string args;
    config_->lookup ("args", &args);
    argv_ = parseArgs (binary, args, &argc_);
    temporalNegotiator_ = new TemporalNegotiator (this);
  }


  Setup::~Setup ()
  {
    for (std::vector<Port*>::iterator i = ports_.begin ();
	 i != ports_.end ();
	 ++i)
      (*i)->setupCleanup ();

    if (launchedByMusic ())
      delete temporalNegotiator_;

    // delete connection objects
    for (std::vector<Connection*>::iterator i = connections_->begin ();
	 i != connections_->end ();
	 ++i)
      delete *i;
    
    delete connections_;
    delete config_;

    isInstantiated_ = false;
  }
  

  bool
  Setup::launchedByMusic ()
  {
    return config_->launchedByMusic ();
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

  
  MessageInputPort*
  Setup::publishMessageInput (std::string identifier)
  {
    return new MessageInputPort (this, identifier);
  }


  MessageOutputPort*
  Setup::publishMessageOutput (std::string identifier)
  {
    return new MessageOutputPort (this, identifier);
  }

  
  void Setup::addPort (Port* p)
  {
    ports_.push_back (p);
  }

  
  void Setup::addConnection (Connection* c)
  {
    connections_->push_back (c);
  }

}

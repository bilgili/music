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
#include "music/setup.hh"
#if MUSIC_USE_MPI
#include <mpi.h>

#include "config.h"


#include "music/runtime.hh"
#include "music/parse.hh"
#include "music/error.hh"

#include <strings.h>

namespace MUSIC {

  bool Setup::isInstantiated_ = false;
  static std::string err_MPI_Init = "MPI_Init was called before the Setup constructor";

  Setup::Setup (int& argc, char**& argv)
    : argc_ (argc), argv_ (argv)
  {
    checkInstantiatedOnce (isInstantiated_, "Setup");
    if (MPI::Is_initialized ())
      errorRank (err_MPI_Init);
    maybeProcessMusicArgv (argc, argv);
    MPI::Init (argc, argv);

    init (argc, argv);

  }

  
  Setup::Setup (int& argc, char**& argv, int required, int* provided)
    : argc_ (argc), argv_ (argv)
  {
    checkInstantiatedOnce (isInstantiated_, "Setup");
    if (MPI::Is_initialized ())
      errorRank (err_MPI_Init);
    maybeProcessMusicArgv (argc, argv);
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
    char *app_node;
/*
 * remedius
 * first argument of each application should contain the name of the application node
 *  described in the configuration file *.music that was used for generating music.map file on BG/P
 */
#if defined(__bgp__)
    app_node = argv[1];
#else
    app_node = NULL;
#endif
    config_ = new Configuration (app_node);

    connections_ = new std::vector<Connection*>; // destroyed by runtime
    if (launchedByMusic ())
      {
	// launched by the music utility
	if (!config_->postponeSetup ())
	  fullInit ();
	comm = MPI::COMM_WORLD.Split (config_->color (), myRank);
	if (!config ("timebase", &timebase_))
	  timebase_ = MUSIC_DEFAULT_TIMEBASE;	       // default timebase
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
	timebase_ = MUSIC_DEFAULT_TIMEBASE;
      }
  }


  void
  Setup::maybeProcessMusicArgv (int& argc, char**& argv)
  {
    char* MUSIC_ARGV = getenv ("MUSIC_ARGV");
    if (MUSIC_ARGV != NULL)
      {
	std::string cmd;
	std::string argstring;
	char* s = index (MUSIC_ARGV, ' ');
	if (s == NULL)
	  {
	    cmd = std::string (MUSIC_ARGV);
	    argstring = "";
	  }
	else
	  {
	    cmd = std::string (MUSIC_ARGV, s - MUSIC_ARGV);
	    argstring = std::string (s + 1);
	  }
	char** newArgv = parseArgs (cmd, argstring, &argc);
	for (int i = 0; i < argc; ++i)
	  argv[i] = newArgv[i];
	delete[] newArgv;
      }
  }


  void
  Setup::maybePostponedSetup ()
  {
    if (config_->postponeSetup ())
      {
	delete config_;
	config_ = new Configuration (0 /*fixme*/);
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

  int
  Setup::applicationColor ()
  {
	  return config_->color();
  }

  std::string
  Setup::applicationName()
  {
    return config_->ApplicationName();
  }

  int
  Setup::leader ()
  {
    return config_->leader ();
  }

  int
  Setup::nProcs ()
  {
    return comm.Get_size ();
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
#endif

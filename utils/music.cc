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

#include <string>
#include <fstream>

#include "config.h"

#include <music/version.hh>

#include "application_mapper.hh"
#include "mpidep/mpidep.hh"

extern "C" {
#include <sys/types.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
}

using std::string;

void
usage (int rank)
{
  if (rank <= 0)
    {
      std::cout << "Usage: mpirun ... music [OPTION...] CONFIG" << std::endl
		<< "`music' launches an application as part of a multi-simulation job." << std::endl << std::endl
		<< "  -h, --help            print this help message" << std::endl
		<< "  -m, --map             print application rank map" << std::endl
		<< "  -e, --export-scripts  export launcher scripts" << std::endl
		<< "  -v, --version         prints version of MUSIC library" << std::endl
		<< std::endl
		<< "Report bugs to <music-bugs@incf.org>." << std::endl;
    }
  exit (0);
}


void
print_map (MUSIC::Configuration* config)
{
  MUSIC::ApplicationMap* a = config->applications ();
  std::cout << "rank\tapplication" << std::endl;
  for (MUSIC::ApplicationMap::iterator i = a->begin (); i != a->end (); ++i)
    {
      int first = i->leader ();
      int nProc = i->nProc ();
      std::cout << first;
      if (nProc > 1)
	std::cout << "-" << first + nProc - 1;
      std::cout << '\t' << i->name () << std::endl;
    }
}

// NOTE: This must match the definition in src/configuration.cc
const static char* const configEnvVarName = "_MUSIC_CONFIG_";

void
export_scripts (MUSIC::ApplicationMapper* map)
{
  MUSIC::Configuration* config = map->config ();
  MUSIC::ApplicationMap* a = config->applications ();
  for (MUSIC::ApplicationMap::iterator i = a->begin (); i != a->end (); ++i)
    {
      std::string name = i->name ();
      std::string fname = name + ".sh";
      std::ofstream script (fname.c_str ());
      script << "#!/bin/sh" << std::endl << std::endl;
      map->mapConnectivity (name);
      MUSIC::Configuration* config = map->config (name);
      config->writeEnv ();
      script << "export " << configEnvVarName
	     << "=\"" << getenv (configEnvVarName)
	     << '"' << std::endl;
      std::string binary;
      config->lookup ("binary", &binary);
      std::string args;
      config->lookup ("args", &args);
      script << binary << ' ' << args << std::endl;
      script.close ();
    }
}


void
print_version (int rank)
{
  if (rank <= 0)
    {
      std::cout << "MUSIC " << MUSIC::version () << std::endl
		<< "Copyright (C) 2007-2011 INCF." << std::endl
		<< "You may redistribute copies of MUSIC" << std::endl
		<< "under the terms of the GNU General Public License." << std::endl
		<< "For more information about these matters, see the file named COPYING." << std::endl;
    }
  exit (0);
}


void
launch (MUSIC::Configuration* config, char** argv)
{
  string binary;
  config->lookup ("binary", &binary);
  config->writeEnv ();
  string wd;
  if (config->lookup ("wd", &wd))
    {
      if (chdir (wd.c_str ()))
	goto error_exit;
    }
  execvp (binary.c_str (), argv);

 error_exit:
  // if we get here, something is wrong
  std::cerr << "Error during launching of binary " << binary << std::endl;
  perror ("MUSIC");
  exit (1);
}


int
main (int argc, char *argv[])
{
  // predict the rank MPI::Init will give us using
  // mpi implementation dependent code from ../mpidep
  int rank = getRank (argc, argv);

  bool do_print_map = false;
  bool do_export_scripts = false;
  
  opterr = 0; // handle errors ourselves
  while (1)
    {
      static struct option longOptions[] =
	{
	  {"help",           no_argument,       0, 'h'},
	  {"map",            required_argument, 0, 'm'},
	  {"export-scripts", no_argument,       0, 'e'},
	  {"version",        no_argument,       0, 'v'},
	  {0, 0, 0, 0}
	};
      /* `getopt_long' stores the option index here. */
      int option_index = 0;

      // the + below tells getopt_long not to reorder argv
      int c = getopt_long (argc, argv, "+hm:ev", longOptions, &option_index);

      /* detect the end of the options */
      if (c == -1)
	break;

      switch (c)
	{
	case '?':
	  break; // ignore unknown options
	case 'h':
	  usage (rank);
	case 'm':
	  do_print_map = true;
	  continue;
	case 'e':
	  do_export_scripts = true;
	  continue;
	case 'v':
	  print_version (rank);

	default:
	  abort ();
	}
    }

  // extract the configuration file name using
  // mpi implementation dependent code from ../mpidep
  std::istream* configFile = getConfig (rank, argc, argv);

  if (!*configFile)
    {
      if (rank <= 0)
	std::cerr << "MUSIC: Couldn't open configuration file "
		  << argv[1] << std::endl;
      exit (1);
    }

  MUSIC::ApplicationMapper map (configFile, rank);

  if (do_print_map)
    {
      if (rank <= 0)
	print_map (map.config ());
    }

  if (do_export_scripts)
    {
      if (rank <= 0)
	export_scripts (&map);
    }

  if (do_print_map || do_export_scripts)
    return 0;
  
  if (rank == -1)
    {
      std::cerr << "MUSIC: Unable to determine process rank." << std::endl
		<< "MUSIC: Did you launch music using mpirun?" << std::endl
		<< "MUSIC: If so, check the comments about porting in README." << std::endl;
      exit (1);
    }

  launch (map.config (), argv);

  return 0;
}

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

#include "config.h"

#include "application_mapper.hh"
#include "mpidep/mpidep.hh"

extern "C" {
#include <sys/types.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
};

using std::string;

void
usage (int rank)
{
  if (rank == 0)
    {
      std::cerr << "Usage: music [OPTION...] CONFIG" << std::endl
		<< "`music' launches an application as part of a multi-simulator job." << std::endl << std::endl
		<< "  -h, --help            print this help message" << std::endl << std::endl
		<< "Report bugs to <mikael@djurfeldt.com>." << std::endl;
    }
  exit (1);
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
  perror ("MUSIC");
  exit (1);
}


int
main (int argc, char *argv[])
{
  // predict the rank MPI::Init will give us using
  // mpi implementation dependent code from ../mpidep
  int rank = getRank (argc, argv);

  opterr = 0; // handle errors ourselves
  while (1)
    {
      static struct option longOptions[] =
	{
	  {"help",    no_argument,       0, 'h'},
	  {0, 0, 0, 0}
	};
      /* `getopt_long' stores the option index here. */
      int option_index = 0;

      // the + below tells getopt_long not to reorder argv
      int c = getopt_long (argc, argv, "+h", longOptions, &option_index);

      /* detect the end of the options */
      if (c == -1)
	break;

      switch (c)
	{
	case '?':
	  break; // ignore unknown options
	case 'h':
	  usage (rank);

	default:
	  abort ();
	}
    }

  // extract the configuration file name using
  // mpi implementation dependent code from ../mpidep
  std::istream* configFile = getConfig (rank, argc, argv);

  if (!*configFile)
    {
      if (rank == 0)
	std::cerr << "MUSIC: Couldn't open config file "
		  << argv[1] << std::endl;
      exit (1);
    }

  MUSIC::ApplicationMapper map (configFile, rank);
  
  launch (map.config (), argv);

  return 0;
}

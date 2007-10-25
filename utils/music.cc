/*
 *  This file is part of MUSIC.
 *  Copyright (C) 2007 CSC, KTH
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
#include <sstream>
#include <fstream>
#include <iostream>

#include "parse.hh"

extern "C" {
  #include <getopt.h>
};

using std::string;
using std::ifstream;

// Implementation-dependent code

int
get_rank (int argc, char *argv[])
{
#ifdef HAVE_RTS_GET_PERSONALITY
  // BG/L
  BGLPersonality p;
  rts_get_personality (&p, sizeof (p));
  int pid = rts_get_processor_id ();
  unsigned int rank, np;
  rts_rankForCoordinates (p.xCoord, p.yCoord, p.zCoord, pid, &rank, &np);
  return rank;
#else
  // mpich
  int rank;
  const std::string rankopt = "-p4rmrank";
  for (int i = argc - 2; i > 0; --i)
    if (argv[i] == rankopt)
      {
	std::istringstream iss (argv[i + 1]);
	iss >> rank;
	return rank;
      }
  return 0;
#endif
}

// Generic code

void
usage (int rank)
{
  if (rank == 0)
    {
      std::cerr << "Usage: music [OPTION...] CONFIG" << std::endl
		<< "`music' launches an application as part of a multi-simulator job." << std::endl << std::endl
		<< "  -h, --help            print this help message" << std::endl << std::endl
		<< "Report bugs to <mikael@djurfeldt.com>." << std::endl;
      exit (1);
    }
}

void
launch (int rank, MUSIC::application_map* map)
{
  MUSIC::configuration* config = map->configuration_for_rank (rank);
  string binary = config->lookup_string ("binary");
  string args = config->lookup_string ("args");
  char **argv = parse_args (args);
  
  map->write_env ();
  execvp (binary.data (), argv);

  // if we get here, something is wrong
  perror ("MUSIC:");
  exit (1);
}

int
main (int argc, char *argv[])
{
  int rank = get_rank (argc, argv);

  opterr = 0; // handle errors ourselves
  while (1)
    {
      static struct option long_options[] =
	{
	  {"help",    no_argument,       0, 'h'},
	  {0, 0, 0, 0}
	};
      /* `getopt_long' stores the option index here. */
      int option_index = 0;

      // the + below tells getopt_long not to reorder argv
      int c = getopt_long (argc, argv, "+h", long_options, &option_index);

      /* detect the end of the options */
      if (c == -1)
	break;

      switch (c)
	{
	case '?':
	  break; // ignore unknown options
	case 'h':
	  usage (rank);
	  break;

	default:
	  abort ();
	}
    }

  // check that we have at least one arg and that this is not an option
  if (argc < 2 || *argv[1] == '-')
    usage (rank);

  ifstream config_file (argv[1]);
  if (!config_file)
    {
      if (rank == 0)
	std::cerr << "Couldn't open config file " << argv[1] << std::endl;
      exit (1);
    }
  
  MUSIC::application_map* map = parse_config (&config_file);

  launch (rank, map);

  return 0;
}

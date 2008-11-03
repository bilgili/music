/*
 *  This file is part of MUSIC.
 *  Copyright (C) 2008 CSC, KTH
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

// Leave as first include---required by BG/L
#include <mpi.h>

#include <iostream>
#include <sstream>
#include <string>
#include <cstdlib>

extern "C" {
#include <unistd.h>
#include <getopt.h>
};

#include <music.hh>

#include "datafile.h"

const double DEFAULT_TIMESTEP = 1e-2;

void
usage (int rank)
{
  if (rank == 0)
    {
      std::cerr << "Usage: eventsource [OPTION...] N_UNITS PREFIX [SUFFIX]" << std::endl
		<< "`eventsource' reads spikes from a set of files with names PREFIX RANK SUFFIX" << std::endl
		<< "and propagates these spikes through a MUSIC output port." << std::endl << std:: endl
		<< "  -t, --timestep TIMESTEP time between tick() calls (default " << DEFAULT_TIMESTEP << " s)" << std::endl
		<< "  -m, --imaptype TYPE     linear (default) or roundrobin" << std::endl
		<< "  -i, --indextype TYPE    global (default) or local" << std::endl
		<< "  -h, --help              print this help message" << std::endl << std::endl
		<< "Report bugs to <mikael@djurfeldt.com>." << std::endl;
    }
  exit (1);
}

int n_units;
double timestep = DEFAULT_TIMESTEP;
string imaptype = "linear";
string indextype = "global";
string prefix;
string suffix = ".dat";

void
getargs (int rank, int argc, char* argv[])
{
  opterr = 0; // handle errors ourselves
  while (1)
    {
      static struct option long_options[] =
	{
	  {"timestep",  required_argument, 0, 't'},
	  {"imaptype",  required_argument, 0, 'm'},
	  {"indextype", required_argument, 0, 'i'},
	  {"help",      no_argument,       0, 'h'},
	  {0, 0, 0, 0}
	};
      /* `getopt_long' stores the option index here. */
      int option_index = 0;

      // the + below tells getopt_long not to reorder argv
      int c = getopt_long (argc, argv, "+t:m:i:h", long_options, &option_index);

      /* detect the end of the options */
      if (c == -1)
	break;

      switch (c)
	{
	case 't':
	  timestep = atof (optarg); //*fixme* error checking
	  continue;
	case 'm':
	  imaptype = optarg;
	  if (imaptype != "linear" && imaptype != "roundrobin")
	    {
	      usage (rank);
	      abort ();
	    }
	  continue;
	case 'i':
	  indextype = optarg;
	  if (indextype != "global" && indextype != "local")
	    {
	      usage (rank);
	      abort ();
	    }
	  continue;
	case '?':
	  break; // ignore unknown options
	case 'h':
	  usage (rank);

	default:
	  abort ();
	}
    }

  if (argc < optind + 2 || argc > optind + 3)
    usage (rank);

  n_units = atoi (argv[optind]);
  prefix = argv[optind + 1];
  if (argc == optind + 3)
    suffix = argv[optind + 2];
}

int
main (int argc, char *argv[])
{
  MUSIC::setup* setup = new MUSIC::setup (argc, argv);
  
  MPI::Intracomm comm = setup->communicator ();
  int n_processes = comm.Get_size ();
  int rank = comm.Get_rank ();
  
  getargs (rank, argc, argv);

  MUSIC::event_output_port* out = setup->publish_event_output ("out");
  if (!out->is_connected ())
    {
      if (rank == 0)
	std::cerr << "eventsource port is not connected" << std::endl;
      exit (1);
    }

  MUSIC::index::type type;
  if (indextype == "global")
    type = MUSIC::index::GLOBAL;
  else
    type = MUSIC::index::LOCAL;
  
  if (imaptype == "linear")
    {
      int n_units_per_process = n_units / n_processes;
      int n_local_units = n_units_per_process;
      int rest = n_units % n_processes;
      int first_id = n_units_per_process * rank;
      if (rank < rest)
	{
	  first_id += rank;
	  n_local_units += 1;
	}
      else
	first_id += rest;
      MUSIC::linear_index indices (first_id, n_local_units);
      
      out->map (&indices, type);
    }
  else
    {
      std::vector<MUSIC::global_index> v;
      for (int i = rank; i < n_units; i += n_processes)
	v.push_back (i);
      MUSIC::permutation_index indices (&v.front (), v.size ());
      out->map (&indices, type);
    }

  double stoptime;
  setup->config ("stoptime", &stoptime);

  std::ostringstream spikefile;
  spikefile << prefix << rank << suffix;
  datafile in (spikefile.str ());
  if (!in)
    {
      std::cerr << "eventsource: could not open "
		<< spikefile.str () << std::endl;
      abort ();      
    }

  MUSIC::runtime* runtime = new MUSIC::runtime (setup, timestep);

  in.skip_header ();
  int id;
  double t;
  in >> t >> id;
  bool more_spikes = !in.eof ();
  
  double time = runtime->time ();
  while (time < stoptime)
    {
      double next_time = time + timestep;
      while (more_spikes && t < next_time)
	{
	  out->insert_event (t, MUSIC::global_index (id));
	  in >> t >> id;
	  more_spikes = !in.eof ();
	}
      // Make data available for other programs
      runtime->tick ();

      time = runtime->time ();
    }

  runtime->finalize ();

  delete runtime;

  return 0;
}

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
#include <fstream>
#include <string>
#include <cstdlib>
#include <vector>

extern "C" {
#include <unistd.h>
#include <getopt.h>
};

#include <music.hh>

const double DEFAULT_TIMESTEP = 1e-2;

void
usage (int rank)
{
  if (rank == 0)
    {
      std::cerr << "Usage: eventcounter [OPTION...] N_UNITS PREFIX [SUFFIX]" << std::endl
		<< "`eventcounter' receives spikes through a MUSIC input port" << std::endl
		<< "and writes these to a set of files with names PREFIX RANK SUFFIX" << std::endl << std:: endl
		<< "  -t, --timestep TIMESTEP time between tick() calls (default " << DEFAULT_TIMESTEP << " s)" << std::endl
		<< "  -m, --imaptype TYPE     linear (default) or roundrobin" << std::endl
		<< "  -h, --help              print this help message" << std::endl << std::endl
		<< "Report bugs to <mikael@djurfeldt.com>." << std::endl;
    }
  exit (1);
}

std::vector<int> counters;

class my_event_handler_local: public MUSIC::event_handler_local_index {
public:
  void operator () (double t, MUSIC::local_index id)
  {
    ++counters[id];
  }
};

int n_units;
double timestep = DEFAULT_TIMESTEP;
string imaptype = "linear";
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
	  {"help",      no_argument,       0, 'h'},
	  {0, 0, 0, 0}
	};
      /* `getopt_long' stores the option index here. */
      int option_index = 0;

      // the + below tells getopt_long not to reorder argv
      int c = getopt_long (argc, argv, "+t:m:h", long_options, &option_index);

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

  MUSIC::event_input_port* in = setup->publish_event_input ("in");
  if (!in->is_connected ())
    {
      if (rank == 0)
	std::cerr << "eventcounter port is not connected" << std::endl;
      exit (1);
    }

  std::ostringstream spikefile;
  spikefile << prefix << rank << suffix;
  std::ofstream out (spikefile.str ().c_str ());
  if (!out)
    {
      std::cerr << "eventcounter: could not open "
		<< spikefile.str () << " for writing" << std::endl;
      abort ();      
    }

  my_event_handler_local evhandler_local;
  
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

      in->map (&indices, &evhandler_local, 0.0);
    }
  else
    {
      std::vector<MUSIC::global_index> v;
      for (int i = rank; i < n_units; i += n_processes)
	v.push_back (i);
      MUSIC::permutation_index indices (&v.front (), v.size ());

      in->map (&indices, &evhandler_local, 0.0);
    }

  counters.resize (n_units);
  for (int i = 0; i < n_units; ++i)
    counters[i] = 0;

  double stoptime;
  setup->config ("stoptime", &stoptime);

  MUSIC::runtime* runtime = new MUSIC::runtime (setup, timestep);

  double time = runtime->time ();
  while (time < stoptime)
    {
      // Retrieve data from other program
      runtime->tick ();
      
      time = runtime->time ();
    }
  for (std::vector<int>::iterator i = counters.begin ();
       i != counters.end ();
       ++i)
    out << static_cast<double> (*i) / stoptime << std::endl;
  out.close ();

  runtime->finalize ();

  delete runtime;

  return 0;
}

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
      std::cerr << "Usage: eventselect [OPTION...] N_UNITS UNITS" << std::endl
		<< "`eventselect' receives events from an input port of width N_UNITS" << std::endl
		<< "and sends events for the subset of id:s specified in the file UNITS" << std::endl << std:: endl
		<< "  -t, --timestep TIMESTEP time between tick() calls (default " << DEFAULT_TIMESTEP << " s)" << std::endl
		<< "  -h, --help              print this help message" << std::endl << std::endl
		<< "Report bugs to <mikael@djurfeldt.com>." << std::endl;
    }
  exit (1);
}

std::vector<MUSIC::event> event_buffer;

class my_event_handler_global : public MUSIC::event_handler_global_index {
public:
  void operator () (double t, MUSIC::global_index id)
  {
    event_buffer.push_back (MUSIC::event (t, id));
  }
};

int n_units;
double timestep = DEFAULT_TIMESTEP;
string units;

void
getargs (int rank, int argc, char* argv[])
{
  opterr = 0; // handle errors ourselves
  while (1)
    {
      static struct option long_options[] =
	{
	  {"timestep",  required_argument, 0, 't'},
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
	case '?':
	  break; // ignore unknown options
	case 'h':
	  usage (rank);

	default:
	  abort ();
	}
    }

  if (argc != optind + 2)
    usage (rank);

  n_units = atoi (argv[optind]);
  units = argv[optind + 1];
}

void
map_output (MUSIC::event_output_port* out,
	    int n_processes,
	    int rank,
	    std::string fname)
{
  std::ifstream unitfile (fname.c_str ());
  if (!unitfile)
    {
      std::cerr << "eventselect: could not open "
		<< fname << " for writing" << std::endl;
      abort ();      
    }

  // First count the integers
  int n_units = 0;
  int id;
  while (unitfile >> id)
    ++n_units;
  unitfile.close ();
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
  // Now read in our selection
  n_units = 0;
  unitfile.open (fname.c_str ());
  while (n_units < first_id)
    {
      unitfile >> id;
      ++n_units;
    }
  n_units = 0;
  std::vector<MUSIC::global_index> units;
  while (n_units < n_local_units)
    {
      unitfile >> id;
      units.push_back (id);
      ++n_units;
    }
  unitfile.close ();
  MUSIC::permutation_index indices (&units.front (), units.size ());

  out->map (&indices, MUSIC::index::GLOBAL);
}

void
map_input (MUSIC::event_input_port* in,
	   int n_processes,
	   int rank,
	   my_event_handler_global& evhandler)
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

  in->map (&indices, &evhandler, 0.0);
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
	std::cerr << "eventselect port in is not connected" << std::endl;
      exit (1);
    }

  MUSIC::event_output_port* out = setup->publish_event_output ("out");
  if (!out->is_connected ())
    {
      if (rank == 0)
	std::cerr << "eventselect port out is not connected" << std::endl;
      exit (1);
    }

  map_output (out, n_processes, rank, units);

  my_event_handler_global evhandler_global;

  map_input (in, n_processes, rank, evhandler_global);
  
  double stoptime;
  setup->config ("stoptime", &stoptime);

  MUSIC::runtime* runtime = new MUSIC::runtime (setup, timestep);

  double time = runtime->time ();
  while (time < stoptime)
    {
      event_buffer.clear ();
      // Retrieve data
      runtime->tick ();
      
      sort (event_buffer.begin (), event_buffer.end ());
      for (std::vector<MUSIC::event>::iterator i = event_buffer.begin ();
	   i != event_buffer.end ();
	   ++i)
	// Send data (*fixme* assumes that non-mapped id:s will be discarded)
	out->insert_event (i->t, MUSIC::global_index (i->id));

      time = runtime->time ();
    }

  runtime->finalize ();

  delete runtime;

  return 0;
}

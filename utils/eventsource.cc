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

#include <music.hh>

#include "datafile.h"

#define TIMESTEP 1e-2

int n_units;
string imaptype;
string prefix;
string suffix;

void
getargs (int argc, char* argv[])
{
  if (argc < 4 || argc > 5 )
    {
      std::cerr << "eventsource N_UNITS IMAPTYPE PREFIX [SUFFIX]" << std::endl;
      exit (1);
    }

  n_units = atoi (argv[1]);
  imaptype = argv[2];
  prefix = argv[3];
  if (argc == 4)
    suffix = ".dat";
  else
    suffix = argv[4];
}

int
main (int argc, char *argv[])
{
  MUSIC::setup* setup = new MUSIC::setup (argc, argv);
  
  getargs (argc, argv);

  MUSIC::event_output_port* out = setup->publish_event_output ("out");

  MPI::Intracomm comm = setup->communicator ();
  int n_processes = comm.Get_size ();
  int rank = comm.Get_rank ();
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
  
  out->map (&indices);
  
  double stoptime;
  setup->config ("stoptime", &stoptime);

  std::ostringstream spikefile;
  spikefile << prefix << rank << suffix;
  datafile in (spikefile.str ());

  MUSIC::runtime* runtime = new MUSIC::runtime (setup, TIMESTEP);

  in.skip_header ();
  int id;
  double t;
  in >> t >> id;
  bool more_spikes = !in.eof ();
  
  double time = runtime->time ();
  while (time < stoptime)
    {
      double next_time = time + TIMESTEP;
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

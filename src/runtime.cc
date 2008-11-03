/*
 *  This file is part of MUSIC.
 *  Copyright (C) 2007, 2008 INCF
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

#include <mpi.h>

#include <algorithm>

#include "music/runtime.hh"
#include "music/temporal.hh"

namespace MUSIC {

  runtime::runtime (setup* s, double h)
  {
    // Setup the MUSIC clock
    double timebase;
    if (!s->config ("timebase", &timebase))
      timebase = 1e-9;		// default timebase
    local_time = clock (timebase, h);
    
    comm = s->communicator ();
    if (s->launched_by_music ())
      {
	spatial_negotiation (s);
	build_tables (s);
	build_schedule (comm.Get_rank ());
	temporal_negotiation (s, timebase, local_time.tick_interval ());
      }
    delete s;
  }


  runtime::~runtime ()
  {
    for (std::vector<output_subconnector*>::iterator c
	   = output_subconnectors.begin ();
	 c != output_subconnectors.end ();
	 ++c)
      delete *c;
    for (std::vector<input_subconnector*>::iterator c
	   = input_subconnectors.begin ();
	 c != input_subconnectors.end ();
	 ++c)
      delete *c;
  }
  

  // This predicate gives a total order for connectors which is the
  // same on the sender and receiver sides.  It belongs here rather
  // than in connector.hh or connector.cc since it is connected to the
  // connect algorithm.
  bool
  less_connector (const connector* c1, const connector* c2)
  {
    return (c1->receiver_app_name () < c2->receiver_app_name ()
	    || (c1->receiver_app_name () == c2->receiver_app_name ()
		&& c1->receiver_port_name () < c2->receiver_port_name ()));
  }
  
  
  // This predicate gives a total order for subconnectors which is the
  // same on the sender and receiver sides.  It belongs here rather
  // than in connector.hh or connector.cc since it is connected to the
  // connect algorithm.
  bool
  less_subconnector (const subconnector* c1, const subconnector* c2)
  {
    return (c1->receiver_rank () < c2->receiver_rank ()
	    || (c1->receiver_rank () == c2->receiver_rank ()
		&& c1->receiver_port_name () < c2->receiver_port_name ()));
  }

  
  void
  runtime::spatial_negotiation (setup* s)
  {
    // Let each connector pair setup their inter-communicators
    // and create all required subconnectors.

    std::vector<connector*>* connectors = s->connectors ();
    // This ordering is necessary so that both sender and receiver
    // in each pair sets up communication at the same point in time
    //
    // Note that we don't need to use the dead-lock scheme used in
    // build_schedule () here.
    //
    sort (connectors->begin (), connectors->end (), less_connector);
    for (std::vector<connector*>::iterator c = connectors->begin ();
	 c != connectors->end ();
	 ++c)
      (*c)->spatial_negotiation (output_subconnectors, input_subconnectors);
  }

  void
  runtime::build_schedule (int local_rank)
  {
    // Build the communication schedule.
    
    // The following communication schedule prevents dead-locking,
    // both during inter-communicator creation and during
    // communication of data.
    //
    // Mikael Djurfeldt et al. (2005) "Massively parallel simulation
    // of brain-scale neuronal network models." Tech. Rep.
    // TRITA-NA-P0513, CSC, KTH, Stockholm.
    
    // First sort connectors according to a total order
    
    sort (output_subconnectors.begin (), output_subconnectors.end (),
	  less_subconnector);
    sort (input_subconnectors.begin (), input_subconnectors.end (),
	  less_subconnector);

    // Now, build up the schedule to be used later during
    // communication of data.
    for (std::vector<input_subconnector*>::iterator c = input_subconnectors.begin ();
	 (c != input_subconnectors.end ()
	  && (*c)->remote_rank () < local_rank);
	 ++c)
      schedule.push_back (*c);
    {
      std::vector<output_subconnector*>::iterator c = output_subconnectors.begin ();
      for (;
	   (c != output_subconnectors.end ()
	    && (*c)->remote_rank () < local_rank);
	   ++c)
	;
      for (; c != output_subconnectors.end (); ++c)
	schedule.push_back (*c);
    }
    for (std::vector<output_subconnector*>::iterator c = output_subconnectors.begin ();
	 (c != output_subconnectors.end ()
	  && (*c)->remote_rank () < local_rank);
	 ++c)
      schedule.push_back (*c);
    {
      std::vector<input_subconnector*>::iterator c = input_subconnectors.begin ();
      for (;
	   (c != input_subconnectors.end ()
	    && (*c)->remote_rank () < local_rank);
	   ++c)
	;
      for (; c != input_subconnectors.end (); ++c)
	schedule.push_back (*c);
    }
  }

  
  void
  runtime::build_tables (setup* s)
  {
    std::vector<port*>* ports = s->ports ();
    for (std::vector<port*>::iterator p = ports->begin ();
	 p != ports->end ();
	 ++p)
      (*p)->build_table ();
  }

  
  void
  runtime::temporal_negotiation (setup* s, double timebase, clock_state_t ti)
  {
    // Temporal negotiation is done globally by a serial algorithm
    // which yields the same result in each process
    temporal_negotiator negotiator (s, timebase, ti);
  }


  MPI::Intracomm
  runtime::communicator ()
  {
    return comm;
  }


  void
  runtime::finalize ()
  {
    MPI::Finalize ();
  }


  void
  runtime::tick ()
  {
    // Loop through the schedule of connectors
    std::vector<subconnector*>::iterator c;
    for (c = schedule.begin (); c != schedule.end (); ++c)
      (*c)->tick ();
    
    local_time.tick ();
  }


  double
  runtime::time ()
  {
    return local_time.time ();
  }
  
}

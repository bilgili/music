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


namespace MUSIC {

  runtime::runtime (setup* s, double h)
    //*fixme* 1e-9
    : local_time (1e-9, h)
  {
    my_communicator = s->communicator ();
    if (s->launched_by_music ())
      {
	connect (s);
      }
    delete s;
  }

  
  bool
  less_remote_leader (const connector* c1, const connector* c2)
  {
    return (c1->remote_leader () < c2->remote_leader ()
	    || (c1->remote_leader () == c2->remote_leader ()
		&& c1->remote_port_name () < c2->remote_port_name ()));
  }

  
  void
  runtime::connect (setup* s)
  {
    // This communication schedule prevents dead-locking,
    // both during inter-communicator creation and
    // during communication of data.
    //
    // Mikael Djurfeldt et al. (2005) "Massively parallel simulation
    // of brain-scale neuronal network models." Tech. Rep.
    // TRITA-NA-P0513, CSC, KTH, Stockholm.

    // First sort connectors according to remote_leader
    std::vector<input_connector*>* iconnectors = s->input_connectors ();
    sort (iconnectors->begin (), iconnectors->end (), less_remote_leader);
    std::vector<output_connector*>* oconnectors = s->output_connectors ();
    sort (oconnectors->begin (), oconnectors->end (), less_remote_leader);

    // Now, connect to other process groups and build up the schedule
    // to be used later during communication of data.
    for (std::vector<input_connector*>::iterator c = iconnectors->begin ();
	 (c != iconnectors->end ()
	  && (*c)->remote_leader () < (*c)->local_leader ());
	 ++c)
      {
	(*c)->connect ();
	schedule.push_back (*c);
      }
    {
      std::vector<output_connector*>::iterator c = oconnectors->begin ();
      for (;
	   (c != oconnectors->end ()
	    && (*c)->remote_leader () < (*c)->local_leader ());
	   ++c)
	;
      for (; c != oconnectors->end (); ++c)
	{
	  (*c)->connect ();
	  schedule.push_back (*c);
	}
    }
    for (std::vector<output_connector*>::iterator c = oconnectors->begin ();
	 (c != oconnectors->end ()
	  && (*c)->remote_leader () < (*c)->local_leader ());
	 ++c)
      {
	(*c)->connect ();
	schedule.push_back (*c);
      }
    {
      std::vector<input_connector*>::iterator c = iconnectors->begin ();
      for (;
	   (c != iconnectors->end ()
	    && (*c)->remote_leader () < (*c)->local_leader ());
	   ++c)
	;
      for (; c != iconnectors->end (); ++c)
	{
	  (*c)->connect ();
	  schedule.push_back (*c);
	}
    }
  }

  
  MPI::Intracomm
  runtime::communicator ()
  {
    return my_communicator;
  }


  void
  runtime::finalize ()
  {
    //my_communicator.Free ();
    MPI::Finalize ();
  }


  void
  runtime::tick ()
  {
    // Loop through the schedule of connectors
    std::vector<connector*>::iterator c;
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


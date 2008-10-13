/*
 *  This file is part of MUSIC.
 *  Copyright (C) 2008 INCF
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

#include "music/connector.hh"
#include "music/event.hh"
#include "music/message.hh"

//*fixme* remove
#include <iostream>

#define DEBUG 0

namespace MUSIC {

  connector::connector (MPI::Intracomm c, int es)
    : comm (c), buffer (es)
  {
    
  }


  void
  connector::connect ()
  {
#if DEBUG
    std::cout << "Process " << MPI::COMM_WORLD.Get_rank () << " creating intercomm with local " << _local_leader << " and remote " << _remote_leader << std::endl;
#endif
    intercomm = comm.Create_intercomm (_local_leader, MPI::COMM_WORLD, _remote_leader, 0);
    partner = intercomm.Get_rank ();
  }


  output_connector::output_connector ()
  {
    //*fixme*
    _local_leader = 0;
    _remote_leader = MPI::COMM_WORLD.Get_size () / 2;
    _remote_port_name = "nisse";
  }

  
  void
  output_connector::send ()
  {
  }

  
  int
  output_connector::start_idx ()
  {
    return 0;
  }

  
  int
  output_connector::end_idx ()
  {
    return 0;
  }

  
  input_connector::input_connector ()
  {
    //*fixme*
    _local_leader = 0;
    _remote_leader = 0;
    _remote_port_name = "nisse";
  }


  event_connector::event_connector ()
  {
    
  }
  

  void
  cont_output_connector::mark ()
  {
  }


  event_output_connector::event_output_connector (MPI::Intracomm c)
    : connector (c, sizeof (event))
  {
  }
  

  void
  event_output_connector::send ()
  {
    void* data;
    int size;
    buffer.next_block (data, size);
    //*fixme* marshalling, routing to multiple partners
#if DEBUG
    std::cout << "Process " << MPI::COMM_WORLD.Get_rank () << " sending " << size << " bytes" << std::endl;
#endif
    intercomm.Send (data, size, MPI::BYTE, partner, MUSIC_SPIKE_MSG);
  }
  

  event_input_connector::event_input_connector (MPI::Intracomm c,
						event_handler_global_index* eh)
    : handle_event (eh), connector (c, sizeof (event))
  {
  }

  
  void
  event_input_connector::receive ()
  {
    int size = 1000; //*fixme*
    char* data[size]; 
    MPI::Status status;
#if DEBUG
    std::cout << "Process " << MPI::COMM_WORLD.Get_rank () << " receiving" << std::endl;
#endif
    intercomm.Recv (data, size, MPI::BYTE, partner, MUSIC_SPIKE_MSG, status);
    int n_events = status.Get_count (MPI::BYTE) / sizeof (event);
#if DEBUG
    std::cout << "Process " << MPI::COMM_WORLD.Get_rank () << " received " << n_events << " events" << std::endl;
#endif
    event* ev = (event*) data;
    for (int i = 0; i < n_events; ++i)
      (*handle_event) (ev[i].t, ev[i].id);
  }
  

  // Connectors

  
  void
  cont_input_connector::receive ()
  {
  }

  
  void
  cont_connector::swap_buffers (cont_data_t*& b1, cont_data_t*& b2)
  {
    cont_data_t* tmp;
    tmp = b1;
    b1 = b2;
    b2 = tmp;
  }
  

  void
  fast_cont_output_connector::interpolate_to (int start, int end, cont_data_t* data)
  {
  }
  
  
  void
  fast_cont_output_connector::interpolate_to_buffers ()
  {
#if 0
    std::vector<output_subconnector*>::iterator i = subconnectors.begin ();
    for (; i != subconnectors.end (); ++i)
      {
	cont_data_t* data = (cont_data_t*) (*i)->buffer.insert ();
	interpolate_to ((*i)->start_idx (), (*i)->end_idx (), data);
      }
#endif
  }

  
  void
  fast_cont_output_connector::mark ()
  {
#if 0
    std::vector<output_subconnector*>::iterator i = subconnectors.begin ();
    for (; i != subconnectors.end (); ++i)
      {
	cont_output_subconnector* subcon = (cont_output_subconnector*) *i;
	subcon->buffer.mark ();
      }
#endif
  }

  
  void
  cont_output_connector::send ()
  {
#if 0
    std::vector<output_subconnector*>::iterator i = subconnectors.begin ();
    for (; i != subconnectors.end (); ++i)
      {
	cont_output_subconnector* subcon = (cont_output_subconnector*) *i;
	subcon->send ();
      }
#endif
  }


  void
  fast_cont_output_connector::application_to (cont_data_t* data)
  {
  }
  
  
  void
  slow_cont_input_connector::to_application ()
  {
  }

  
  void
  fast_cont_output_connector::tick ()
  {
    if (synch.sample ())
      {
	swap_buffers (prev_sample, sample);
	// data is copied into sections destined to different subconnectors
	application_to (sample);
	interpolate_to_buffers ();
      }
    if (synch.mark ())
      mark ();
    if (synch.communicate ())
      send ();
  }


  void
  slow_cont_input_connector::buffers_to_application ()
  {
  }

  
  void
  slow_cont_input_connector::tick ()
  {
    if (synch.communicate ())
      receive ();
    buffers_to_application ();
  }


  void
  slow_cont_output_connector::application_to_buffers ()
  {
  }

  
  void
  slow_cont_output_connector::tick ()
  {
    application_to_buffers ();
    if (synch.communicate ())
      send ();
  }


  void
  fast_cont_input_connector::buffers_to (cont_data_t* data)
  {
  }

  
  void
  fast_cont_input_connector::interpolate_to_application ()
  {
  }

  
  void
  fast_cont_input_connector::tick ()
  {
    if (synch.communicate ())
      receive ();
    swap_buffers (prev_sample, sample);
    buffers_to (sample);
    interpolate_to_application ();
  }


  void
  event_output_connector::tick ()
  {
    if (synch.communicate ())
      send ();
  }


  void
  event_input_connector::tick ()
  {
    if (synch.communicate ())
      receive ();    
  }
}

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

#include "music/subconnector.hh"

namespace MUSIC {

  subconnector::subconnector (synchronizer* _synch,
			      MPI::Intercomm _intercomm,
			      int remote_rank,
			      int receiver_rank,
			      std::string receiver_port_name)
    : synch (_synch),
      intercomm (_intercomm),
      _remote_rank (remote_rank),
      _receiver_rank (receiver_rank),
      _receiver_port_name (receiver_port_name)
  {
  }


  subconnector::~subconnector ()
  {
  }

  
  void
  subconnector::connect ()
  {
#if 0
    std::cout << "Process " << MPI::COMM_WORLD.Get_rank () << " creating intercomm with local " << _local_leader << " and remote " << _remote_leader << std::endl;
#endif
  }


  output_subconnector::output_subconnector (synchronizer* synch,
					    MPI::Intercomm intercomm,
					    int remote_rank,
					    int receiver_rank,
					    std::string receiver_port_name,
					    int element_size)
    : subconnector (synch,
		    intercomm,
		    remote_rank,
		    receiver_rank,
		    receiver_port_name),
      _buffer (element_size)
  {
  }

  
  void
  output_subconnector::send ()
  {
  }

  
  int
  output_subconnector::start_idx ()
  {
    return 0;
  }

  
  int
  output_subconnector::end_idx ()
  {
    return 0;
  }

  
  input_subconnector::input_subconnector ()
  {
  }


  event_subconnector::event_subconnector ()
  {
    
  }
  

  void
  cont_output_subconnector::mark ()
  {
  }


  event_output_subconnector::event_output_subconnector (synchronizer* _synch,
							MPI::Intercomm _intercomm,
							int remote_rank,
							std::string _receiver_port_name)
    : subconnector (_synch,
		    _intercomm,
		    remote_rank,
		    remote_rank,
		    _receiver_port_name),
      output_subconnector (_synch,
			   _intercomm,
			   remote_rank,
			   remote_rank, // receiver_rank same as remote rank
			   _receiver_port_name,
			   sizeof (event))
  {
  }
  

  void
  event_output_subconnector::tick ()
  {
    if (synch->communicate ())
      send ();
  }


  void
  event_output_subconnector::send ()
  {
    void* data;
    int size;
    _buffer.next_block (data, size);
    //*fixme* marshalling, routing to multiple partners
#if MUSIC_DEBUG
    std::cout << "Process " << MPI::COMM_WORLD.Get_rank () << " sending " << size << " bytes" << std::endl;
#endif
    intercomm.Send (data, size, MPI::BYTE, _remote_rank, MUSIC_SPIKE_MSG);
  }
  

  event_input_subconnector::event_input_subconnector (synchronizer* synch,
						      MPI::Intercomm intercomm,
						      int remote_rank,
						      int receiver_rank,
						      std::string receiver_port_name)
    : subconnector (synch,
		    intercomm,
		    remote_rank,
		    receiver_rank,
		    receiver_port_name)
  {
  }


  event_input_subconnector_global::event_input_subconnector_global
  (synchronizer* synch,
   MPI::Intercomm intercomm,
   int remote_rank,
   int receiver_rank,
   std::string receiver_port_name,
   event_handler_global_index* eh)
    : subconnector (synch,
		    intercomm,
		    remote_rank,
		    receiver_rank,
		    receiver_port_name),
      event_input_subconnector (synch,
				intercomm,
				remote_rank,
				receiver_rank,
				receiver_port_name),
      handle_event (eh)
  {
  }

  
  event_input_subconnector_local::event_input_subconnector_local
  (synchronizer* synch,
   MPI::Intercomm intercomm,
   int remote_rank,
   int receiver_rank,
   std::string receiver_port_name,
   event_handler_local_index* eh)
    : subconnector (synch,
		    intercomm,
		    remote_rank,
		    receiver_rank,
		    receiver_port_name),
      event_input_subconnector (synch,
				intercomm,
				remote_rank,
				receiver_rank,
				receiver_port_name),
      handle_event (eh)
  {
  }

  
  void
  event_input_subconnector::tick ()
  {
    if (synch->communicate ())
      receive ();    
  }


  //*fixme* isolate difference between global and local
  void
  event_input_subconnector_global::receive ()
  {
    int size = 10000; //*fixme*
    char* data[size]; 
    MPI::Status status;
#if MUSIC_DEBUG
    std::cout << "Process " << MPI::COMM_WORLD.Get_rank () << " receiving" << std::endl;
#endif
    intercomm.Recv (data, size, MPI::BYTE, _remote_rank, MUSIC_SPIKE_MSG, status);
    int n_events = status.Get_count (MPI::BYTE) / sizeof (event);
#if MUSIC_DEBUG
    std::cout << "Process " << MPI::COMM_WORLD.Get_rank () << " received " << n_events << " events" << std::endl;
#endif
    event* ev = (event*) data;
    for (int i = 0; i < n_events; ++i)
      (*handle_event) (ev[i].t, ev[i].id);
  }


  void
  event_input_subconnector_local::receive ()
  {
    int size = 10000; //*fixme*
    char* data[size]; 
    MPI::Status status;
#if MUSIC_DEBUG
    std::cout << "Process " << MPI::COMM_WORLD.Get_rank () << " receiving" << std::endl;
#endif
    intercomm.Recv (data, size, MPI::BYTE, _remote_rank, MUSIC_SPIKE_MSG, status);
    int n_events = status.Get_count (MPI::BYTE) / sizeof (event);
#if MUSIC_DEBUG
    std::cout << "Process " << MPI::COMM_WORLD.Get_rank () << " received " << n_events << " events" << std::endl;
#endif
    event* ev = (event*) data;
    for (int i = 0; i < n_events; ++i)
      (*handle_event) (ev[i].t, ev[i].id);
  }

}

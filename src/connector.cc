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

//#define MUSIC_DEBUG 1

#ifdef MUSIC_DEBUG
#define MUSIC_LOG(X) (std::cout << X << std::endl)
#define MUSIC_LOGN(N, X) { if (MPI::COMM_WORLD.Get_rank () == N) std::cout << X << std::endl; }
#define MUSIC_LOG0(X) MUSIC_LOGN (0, X)
#else
#define MUSIC_LOG(X)
#define MUSIC_LOGN(N, X)

#define MUSIC_LOG0(X)
#endif

namespace MUSIC {

  connector::connector (connector_info _info,
			spatial_negotiator* _negotiator,
			MPI::Intracomm c)
    : info (_info),
      negotiator (_negotiator),
      comm (c)
  {
  }

  
  MPI::Intercomm
  connector::create_intercomm ()
  {
    return comm.Create_intercomm (0,
				  MPI::COMM_WORLD, //*fixme* recursive?
				  info.remote_leader (),
				  0); //*fixme* tag
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
  cont_input_connector::receive ()
  {
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


  event_connector::event_connector (connector_info _info,
				    spatial_negotiator* _negotiator,
				    MPI::Intracomm c)
    : connector (_info, _negotiator, c)
  {
  }
  

  event_output_connector::event_output_connector (connector_info conn_info,
						  spatial_output_negotiator* _negotiator,
						  int max_buffered,
						  MPI::Intracomm comm,
						  event_router& _router)
    : connector (conn_info, _negotiator, comm),
      event_connector (conn_info, _negotiator, comm),
      router (_router)
  {
  }

  
  void
  event_output_connector::spatial_negotiation
  (std::vector<output_subconnector*>& osubconn,
   std::vector<input_subconnector*>& isubconn)
  {
    std::map<int, event_output_subconnector*> subconnectors;
    MPI::Intercomm intercomm = create_intercomm ();
    for (negotiation_iterator i = negotiator->negotiate (comm,
							 intercomm,
							 info.n_processes ());
	 !i.end ();
	 ++i)
      {
	std::map<int, event_output_subconnector*>::iterator c
	  = subconnectors.find (i->rank ());
	event_output_subconnector* subconn;
	if (c != subconnectors.end ())
	  subconn = c->second;
	else
	  {
	    subconn = new event_output_subconnector (&synch,
						     intercomm,
						     i->rank (),
						     receiver_port_name ());
	    subconnectors.insert (std::make_pair (i->rank (), subconn));
	    osubconn.push_back (subconn);
	  }
	MUSIC_LOG (MPI::COMM_WORLD.Get_rank ()
		   << ": ("
		   << i->begin () << ", "
		   << i->end () << ", "
		   << i->local () << ") -> " << i->rank ());
	router.insert_routing_interval (i->interval (), subconn->buffer ());
      }
  }

  
  event_input_connector::event_input_connector (connector_info conn_info,
						spatial_input_negotiator* negotiator,
						event_handler_ptr _handle_event,
						index::type _type,
						double acc_latency,
						int max_buffered,
						MPI::Intracomm comm)
    : connector (conn_info, negotiator, comm),
      event_connector (conn_info, negotiator, comm),
      handle_event (_handle_event),
      type (_type)
  {
  }

  
  void
  event_input_connector::spatial_negotiation
  (std::vector<output_subconnector*>& osubconn,
   std::vector<input_subconnector*>& isubconn)
  {
    std::map<int, event_input_subconnector*> subconnectors;
    MPI::Intercomm intercomm = create_intercomm ();
    int receiver_rank = intercomm.Get_rank ();
    for (negotiation_iterator i = negotiator->negotiate (comm,
							 intercomm,
							 info.n_processes ());
	 !i.end ();
	 ++i)
      {
	std::map<int, event_input_subconnector*>::iterator c
	  = subconnectors.find (i->rank ());
	event_input_subconnector* subconn;
	if (c != subconnectors.end ())
	  subconn = c->second;
	else
	  {
	    if (type == index::GLOBAL)
	      subconn
		= new event_input_subconnector_global (&synch,
						       intercomm,
						       i->rank (),
						       receiver_rank,
						       receiver_port_name (),
						       handle_event.global ());
	    else
	      subconn
		= new event_input_subconnector_local (&synch,
						      intercomm,
						      i->rank (),
						      receiver_rank,
						      receiver_port_name (),
						      handle_event.local ());
	    subconnectors.insert (std::make_pair (i->rank (), subconn));
	    isubconn.push_back (subconn);
	  }
	MUSIC_LOG (MPI::COMM_WORLD.Get_rank ()
		   << ": " << i->rank () << " -> ("
		   << i->begin () << ", "
		   << i->end () << ", "
		   << i->local () << ")");
      }
  }

}

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

#ifndef MUSIC_CONNECTOR_HH

#include <mpi.h>

#include <vector>
#include <string>

#include <music/synchronizer.hh>
#include <music/FIBO.hh>
#include <music/event.hh>
#include <music/spatial.hh>
#include <music/connectivity.hh>
#include <music/event_router.hh>

#include <music/subconnector.hh>

namespace MUSIC {

  typedef char cont_data_t;

  // The connector is responsible for one side of the communication
  // between the ports of a port pair.  An output port can have
  // multiple connectors while an input port only has one.  The method
  // connector::connect () creates one subconnector for each MPI
  // process we will communicate with on the remote side.

  class connector {
  protected:
    connector_info info;
    spatial_negotiator* negotiator;
    MPI::Intracomm comm;
    synchronizer synch;
  public:
    connector () { }
    connector (connector_info _info,
	       spatial_negotiator* _negotiator,
	       MPI::Intracomm c);
    std::string receiver_app_name () const
    { return info.receiver_app_name (); }
    std::string receiver_port_name () const
    { return info.receiver_port_name (); }
    MPI::Intercomm create_intercomm ();
    virtual void
    spatial_negotiation (std::vector<output_subconnector*>& osubconn,
			 std::vector<input_subconnector*>& isubconn) { }
  };

  class output_connector : virtual public connector {
  public:
#if 0
    std::vector<output_subconnector*> subconnectors;
#endif
  };
  
  class input_connector : virtual public connector {
  protected:
  };

  class cont_connector : virtual public connector {
  public:
    void swap_buffers (cont_data_t*& b1, cont_data_t*& b2);
  };  
  
  class fast_connector : virtual public connector {
  protected:
    cont_data_t* prev_sample;
    cont_data_t* sample;
  };
  
  class cont_output_connector : public cont_connector, output_connector {
  public:
    void send ();
  };
  
  class cont_input_connector : public cont_connector, input_connector {
  protected:
    void receive ();
  public:
  };
  
  class fast_cont_output_connector : public cont_output_connector, fast_connector {
    void interpolate_to (int start, int end, cont_data_t* data);
    void interpolate_to_buffers ();
    void application_to (cont_data_t* data);
    void mark ();
  public:
    void tick ();
  };
  
  class slow_cont_input_connector : public cont_input_connector {
  private:
    void buffers_to_application ();
    void to_application ();
  public:
    void tick ();
  };
  
  class slow_cont_output_connector : public cont_output_connector {
    void application_to_buffers ();
  public:
    void tick ();
  };
  
  class fast_cont_input_connector : public cont_input_connector, fast_connector {
    void buffers_to (cont_data_t* data);
    void interpolate_to_application ();
  public:
    void tick ();
  };

  class event_connector : virtual public connector {
  public:
    event_connector (connector_info info,
		     spatial_negotiator* negotiator,
		     MPI::Intracomm c);
  };
  
  class event_output_connector : public output_connector, public event_connector {
    event_router& router;
    void send ();
  public:
    event_output_connector (connector_info conn_info,
			    spatial_output_negotiator* negotiator,
			    int max_buffered,
			    MPI::Intracomm comm,
			    event_router& router);
    void spatial_negotiation (std::vector<output_subconnector*>& osubconn,
			      std::vector<input_subconnector*>& isubconn);
    void tick ();
  };
  
  class event_input_connector : public input_connector, public event_connector {
    
  private:
    event_handler_ptr handle_event;
    index::type type;
  public:
    event_input_connector (connector_info conn_info,
			   spatial_input_negotiator* negotiator,
			   event_handler_ptr handle_event,
			   index::type type,
			   double acc_latency,
			   int max_buffered,
			   MPI::Intracomm comm);
    void spatial_negotiation (std::vector<output_subconnector*>& osubconn,
			      std::vector<input_subconnector*>& isubconn);
  };
  
}

#define MUSIC_CONNECTOR_HH
#endif

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

#ifndef MUSIC_SUBCONNECTOR_HH

#include <mpi.h>

#include <string>

#include <music/synchronizer.hh>
#include <music/FIBO.hh>
#include <music/event.hh>

namespace MUSIC {

  const int MUSIC_SPIKE_MSG = 1;

  // The subconnector is responsible for the local side of the
  // communication between two MPI processes, one for each port of a
  // port pair.  It is created in connector::connect ().
  
  class subconnector {
  private:
  protected:
    synchronizer* synch;
    MPI::Intercomm intercomm;
    int _remote_rank;
    int _receiver_rank;
    std::string _receiver_port_name;
  public:
    subconnector () { }
    subconnector (synchronizer* synch,
		  MPI::Intercomm intercomm,
		  int remote_rank,
		  int receiver_rank,
		  std::string receiver_port_name);
    virtual ~subconnector ();
    virtual void tick () = 0;
    int remote_rank () const { return _remote_rank; }
    int receiver_rank () const { return _receiver_rank; }
    std::string receiver_port_name () const { return _receiver_port_name; }
    void connect ();
  };
  
  class output_subconnector : virtual public subconnector {
  protected:
    FIBO _buffer;
  public:
    output_subconnector (synchronizer* synch,
			 MPI::Intercomm intercomm,
			 int remote_rank,
			 int receiver_rank,
			 std::string receiver_port_name,
			 int element_size);
    FIBO* buffer () { return &_buffer; }
    void send ();
    int start_idx ();
    int end_idx ();
  };
  
  class input_subconnector : virtual public subconnector {
  public:
    input_subconnector ();
  };

  class cont_output_subconnector : public output_subconnector {
  public:
    void mark ();
  };
  
  class cont_input_subconnector : public input_subconnector {
  };

  class event_subconnector : virtual public subconnector {
  public:
    event_subconnector ();
  };
  
  class event_output_subconnector : public output_subconnector {
  public:
    event_output_subconnector (synchronizer* _synch,
			       MPI::Intercomm _intercomm,
			       int remote_rank,
			       std::string _receiver_port_name);
    void tick ();
    void send ();
  };
  
  class event_input_subconnector : public input_subconnector {
  public:
    event_input_subconnector (synchronizer* synch,
			      MPI::Intercomm intercomm,
			      int remote_rank,
			      int receiver_rank,
			      std::string receiver_port_name);
    void tick ();
    virtual void receive () = 0;
  };

  class event_input_subconnector_global : public event_input_subconnector {
    event_handler_global_index* handle_event;
  public:
    event_input_subconnector_global (synchronizer* synch,
				     MPI::Intercomm intercomm,
				     int remote_rank,
				     int receiver_rank,
				     std::string receiver_port_name,
				     event_handler_global_index* eh);
    void receive ();
  };

  class event_input_subconnector_local : public event_input_subconnector {
    event_handler_local_index* handle_event;
  public:
    event_input_subconnector_local (synchronizer* synch,
				    MPI::Intercomm intercomm,
				    int remote_rank,
				    int receiver_rank,
				    std::string receiver_port_name,
				    event_handler_local_index* eh);
    void receive ();
  };

}

#define MUSIC_SUBCONNECTOR_HH
#endif

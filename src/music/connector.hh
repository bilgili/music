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

namespace MUSIC {

  const int MUSIC_SPIKE_MSG = 1;

  typedef char cont_data_t;

  class connector {
  private:
  protected:
    synchronizer synch;
    MPI::Intracomm comm;
    MPI::Intercomm intercomm;
    int partner;
    int _local_leader;
    int _remote_leader;
    std::string _remote_port_name;
  public:
    connector () { }
    connector (MPI::Intracomm c, int element_size);
    virtual void tick () = 0;
    FIBO buffer;//*fixme* move
    int local_leader () const { return _local_leader; }
    int remote_leader () const { return _remote_leader; }
    std::string remote_port_name () const { return _remote_port_name; }
    void connect ();
  };
  
  class output_connector : virtual public connector {
  public:
    output_connector ();
    void send ();
    int start_idx ();
    int end_idx ();
  };
  
  class input_connector : virtual public connector {
  public:
    input_connector ();
  };

  class cont_connector : virtual public connector {
  public:
    void swap_buffers (cont_data_t*& b1, cont_data_t*& b2);
  };
  
  class cont_output_connector : virtual public output_connector, public cont_connector {
  protected:
    void mark ();
    void send ();
  };
  
  class fast_connector : virtual public connector {
  protected:
    cont_data_t* prev_sample;
    cont_data_t* sample;
  };
  
  class fast_cont_output_connector : public cont_output_connector, fast_connector {
    void interpolate_to (int start, int end, cont_data_t* data);
    void interpolate_to_buffers ();
    void application_to (cont_data_t* data);
    void mark ();
  public:
    void tick ();
  };
  
  class cont_input_connector : virtual public input_connector, public cont_connector {
  protected:
    void receive ();
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
    event_connector ();
  };
  
  class event_output_connector : public output_connector {
  public:
    event_output_connector (MPI::Intracomm c);
    void mark ();
    void send ();
    void tick ();
  };
  
  class event_input_connector : public input_connector {
  private:
    event_handler_global_index* handle_event;
    void receive ();
  public:
    event_input_connector (MPI::Intracomm c, event_handler_global_index* eh);
    void tick ();
  };
  
}

#define MUSIC_CONNECTOR_HH
#endif

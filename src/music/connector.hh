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

#include <vector>

#include "music/synchronizer.hh"
#include "music/FIBO.hh"

namespace MUSIC {

  typedef char cont_data_t;

  class subconnector {
  public:
    subconnector ();
    FIBO buffer;
  };
  
  class output_subconnector : public subconnector {
  public:
    output_subconnector ();
    void send ();
    int start_idx ();
    int end_idx ();
  };
  
  class input_subconnector : public subconnector {
  };
  
  class cont_output_subconnector : public output_subconnector {
  public:
    void mark ();
  };
  
  class cont_input_subconnector : public input_subconnector {
  };
  
  class event_output_subconnector : public output_subconnector {
  };
  
  class event_input_subconnector : public input_subconnector {
  };
  
  class connector {
  protected:
    synchronizer synch;
    std::vector<subconnector*> subconnectors;
  public:
    virtual void tick () = 0;
  };

  class output_connector : virtual public connector {
  };
  
  class input_connector : virtual public connector {
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
  
  class event_output_connector : public output_connector {
  public:
    void tick ();
  };
  
  class event_input_connector : public input_connector {
  public:
    void tick ();
  };
  
}

#define MUSIC_CONNECTOR_HH
#endif

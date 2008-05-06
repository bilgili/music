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

namespace MUSIC {

  class subconnector {
  };
  
  class output_subconnector : public subconnector {
  public:
    void send ();
  };
  
  class input_subconnector : public subconnector {
  };
  
  class cont_output_subconnector : public output_subconnector {
  };
  
  class cont_input_subconnector : public input_subconnector {
  };
  
  class event_output_subconnector : public output_subconnector {
  };
  
  class event_input_subconnector : public input_subconnector {
  };
  
  class connector {
    std::vector<subconnector*> subconnectors;
  public:
    virtual void tick () = 0;
  };

  class output_connector : public connector {
  };
  
  class input_connector : public connector {
  };

  class fast_connector : public connector {
  };
  
  class cont_output_connector : public output_connector {
  public:
    void send ();
  };
  
  class cont_input_connector : public input_connector {
  public:
  };
  
  class fast_cont_output_connector : public cont_output_connector {
    void interpolate_to_buffers ();
    void mark ();
  public:
    void tick ();
  };
  
  class slow_cont_input_connector : public cont_input_connector {
  public:
    void tick ();
  };
  
  class slow_cont_output_connector : public cont_output_connector {
  public:
    void tick ();
  };
  
  class fast_cont_input_connector : public cont_input_connector {
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

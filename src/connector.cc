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

namespace MUSIC {

  subconnector::subconnector ()
  {
  }


  output_subconnector::output_subconnector ()
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


  void
  cont_connector::swap_buffers (cont_data_t*& b1, cont_data_t*& b2)
  {
    cont_data_t* tmp;
    tmp = b1;
    b1 = b2;
    b2 = tmp;
  }
  

  void
  cont_output_subconnector::mark ()
  {
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
    std::vector<subconnector*>::iterator i = subconnectors.begin ();
    for (; i != subconnectors.end (); ++i)
      {
	output_subconnector* subcon = (output_subconnector*) *i;
	cont_data_t* data = (cont_data_t*) subcon->buffer.insert ();
	interpolate_to (subcon->start_idx (), subcon->end_idx (), data);
      }
  }

  
  void
  fast_cont_output_connector::mark ()
  {
    std::vector<subconnector*>::iterator i = subconnectors.begin ();
    for (; i != subconnectors.end (); ++i)
      {
	cont_output_subconnector* subcon = (cont_output_subconnector*) *i;
	subcon->buffer.mark ();
      }
  }

  
  void
  cont_output_connector::send ()
  {
    std::vector<subconnector*>::iterator i = subconnectors.begin ();
    for (; i != subconnectors.end (); ++i)
      {
	cont_output_subconnector* subcon = (cont_output_subconnector*) *i;
	subcon->send ();
      }
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
  }

  
  void
  event_input_connector::tick ()
  {
  }
}

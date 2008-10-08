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

namespace MUSIC {

  connector::connector (int es)
    : buffer (es)
  {
  }


  output_connector::output_connector ()
  {
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


  event_connector::event_connector ()
    : connector (sizeof (event))
  {
    
  }
  

  void
  cont_output_connector::mark ()
  {
  }
  

  void
  event_output_connector::send ()
  {
    cont_data_t* data;
    int size;
    buffer.next_block (data, size);
    //*fixme* marshalling
    comm.Send (data, size, MPI::BYTE, partner, MUSIC_SPIKE_MSG);
  }
  

  void
  event_input_connector::receive ()
  {
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

/*
 *  This file is part of MUSIC.
 *  Copyright (C) 2008 CSC, KTH
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

#include "music.hh"

extern "C" {

#include "music-c.h"

/* Setup */

MUSIC_setup *
MUSIC_create_setup (int *argc, char ***argv)
{
  return (MUSIC_setup *) new MUSIC::setup (*argc, *argv);
}


/* Communicators */

MPI_Comm
MUSIC_setup_communicator_glue (MUSIC_setup *setup)
{
  MUSIC::setup* cc_setup = (MUSIC::setup *) setup;
  return (MPI_Comm) cc_setup->communicator ();
}


/* Port creation */

MUSIC_cont_output_port *
MUSIC_publish_cont_output (MUSIC_setup *setup, char *id)
{
  MUSIC::setup* cc_setup = (MUSIC::setup *) setup;
  return (MUSIC_cont_output_port *) cc_setup->publish_cont_output (id);
}


MUSIC_cont_input_port *
MUSIC_publish_cont_input (MUSIC_setup *setup, char *id)
{
  MUSIC::setup* cc_setup = (MUSIC::setup *) setup;
  return (MUSIC_cont_input_port *) cc_setup->publish_cont_input(id);
}


MUSIC_event_output_port *
MUSIC_publish_event_output (MUSIC_setup *setup, char *id)
{
  MUSIC::setup* cc_setup = (MUSIC::setup *) setup;
  return (MUSIC_event_output_port *) cc_setup->publish_event_output(id);
}


MUSIC_event_input_port *
MUSIC_publish_event_input (MUSIC_setup *setup, char *id)
{
  MUSIC::setup* cc_setup = (MUSIC::setup *) setup;
  return (MUSIC_event_input_port *) cc_setup->publish_event_input(id);
}


MUSIC_message_output_port *
MUSIC_publish_message_output (MUSIC_setup *setup, char *id)
{
  MUSIC::setup* cc_setup = (MUSIC::setup *) setup;
  return (MUSIC_message_output_port *) cc_setup->publish_message_output(id);
}


MUSIC_message_input_port *
MUSIC_publish_message_input (MUSIC_setup *setup, char *id)
{
  MUSIC::setup* cc_setup = (MUSIC::setup *) setup;
  return (MUSIC_message_input_port *) cc_setup->publish_message_input(id);
}


void
MUSIC_destroy_cont_output (MUSIC_cont_output_port* port)
{
}


void
MUSIC_destroy_cont_input (MUSIC_cont_input_port* port)
{
}


void
MUSIC_destroy_event_output (MUSIC_event_output_port* port)
{
}


void
MUSIC_destroy_event_input (MUSIC_event_input_port* port)
{
}


void
MUSIC_destroy_message_output (MUSIC_message_output_port* port)
{
}


void
MUSIC_destroy_message_input (MUSIC_message_input_port* port)
{
}


/* General port methods */

int
MUSIC_cont_output_port_is_connected (MUSIC_cont_output_port *port)
{
}


int
MUSIC_cont_input_port_is_connected (MUSIC_cont_input_port *port)
{
}


int
MUSIC_event_output_port_is_connected (MUSIC_event_output_port *port)
{
}


int
MUSIC_event_input_port_is_connected (MUSIC_event_input_port *port)
{
}


int
MUSIC_message_output_port_is_connected (MUSIC_message_output_port *port)
{
}


int
MUSIC_message_input_port_is_connected (MUSIC_message_input_port *port)
{
}


int
MUSIC_cont_output_port_has_width (MUSIC_cont_output_port *port)
{
}


int
MUSIC_cont_input_port_has_width (MUSIC_cont_input_port *port)
{
}


int
MUSIC_event_output_port_has_width (MUSIC_event_output_port *port)
{
}


int
MUSIC_event_input_port_has_width (MUSIC_event_input_port *port)
{
}


int
MUSIC_cont_output_port_width (MUSIC_cont_output_port *port)
{
}


int
MUSIC_cont_input_port_width (MUSIC_cont_input_port *port)
{
}


int
MUSIC_event_output_port_width (MUSIC_event_output_port *port)
{
}


int
MUSIC_event_input_port_width (MUSIC_event_input_port *port)
{
}


/* Mapping */

/* No arguments are optional. */

void
MUSIC_cont_output_port_map (MUSIC_cont_output_port *port,
			    MUSIC_cont_data *dmap,
			    int max_buffered)
{
}


void
MUSIC_cont_input_port_map (MUSIC_cont_input_port *port,
			   MUSIC_cont_data *dmap,
			   double delay,
			   int max_buffered,
			   int interpolate)
{
}


void
MUSIC_event_output_port_map (MUSIC_event_output_port *port,
			     MUSIC_index_map *indices,
			     int max_buffered)
{
}


typedef void MUSIC_event_handler (double t, int id);

void
MUSIC_event_input_port_map (MUSIC_event_input_port *port,
			    MUSIC_index_map *indices,
			    MUSIC_event_handler *handle_event,
			    double acc_latency,
			    int max_buffered)
{
}


void
MUSIC_message_output_port_map (MUSIC_message_output_port *port,
			       int max_buffered)
{
}


typedef void MUSIC_message_handler (double t, void *msg, size_t size);

void
MUSIC_message_input_port_map (MUSIC_message_handler *handle_message,
			      double acc_latency,
			      int max_buffered)
{
}


/* Index maps */

MUSIC_permutation_index *
MUSIC_create_permutation_index (int *indices,
				int size)
{
}


void
MUSIC_destroy_permutation_index (MUSIC_permutation_index *index)
{
}


MUSIC_linear_index *
MUSIC_create_linear_index (int base_index,
			   int size)
{
}


void
MUSIC_destroy_linear_index (MUSIC_linear_index *index)
{
}


/* Data maps */

/* Exception: The map argument can take any type of index map. */

MUSIC_array_data *
MUSIC_create_array_data (void *buffer,
			 MPI_Datatype type,
			 void *map)
{
}


/* Exception: MUSIC_create_linear_array_data corresponds to
   c++ music::array_data::array_data (..., ..., ..., ...) */

MUSIC_array_data *
MUSIC_create_linear_array_data (void *buffer,
				MPI_Datatype type,
				int base_index,
				int size)
{
}


void
MUSIC_destroy_array_data (MUSIC_array_data *array_data)
{
}


/* Configuration variables */

/* Exceptions: Result is char *
   Extra maxlen argument prevents buffer overflow.
   Result is terminated by \0 unless longer than maxlen - 1 */

int
MUSIC_config_string (MUSIC_setup *setup,
		     char *name,
		     char *result,
		     size_t maxlen)
{
}


int
MUSIC_config_int (MUSIC_setup *setup, char *name, int *result)
{
}


int
MUSIC_config_double (MUSIC_setup *setup, char *name, double *result)
{
}


/* Runtime */

MUSIC_runtime *
MUSIC_create_runtime (MUSIC_setup *setup, double h)
{
}


void
MUSIC_tick (MUSIC_runtime *runtime)
{
}


double
MUSIC_time (MUSIC_runtime *runtime)
{
}


/* Finalization */

void
MUSIC_destroy_runtime (MUSIC_runtime *runtime)
{
}

}

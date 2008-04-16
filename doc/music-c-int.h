/* Setup */

MUSIC_setup *MUSIC_create_setup (int *argc, char ***argv);

/* Communicators */

MPI_Intracomm MUSIC_setup_communicator (MUSIC_setup *setup);

/* Port creation */

MUSIC_cont_output_port *MUSIC_publish_cont_output (char *id);
MUSIC_cont_input_port *MUSIC_publish_cont_input (char *id);
MUSIC_event_output_port *MUSIC_publish_event_output (char *id);
MUSIC_event_input_port *MUSIC_publish_event_input (char *id);
MUSIC_message_output_port *MUSIC_publish_message_output (char *id);
MUSIC_message_input_port *MUSIC_publish_message_input (char *id);

void MUSIC_destroy_cont_output (MUSIC_cont_output_port* port);
void MUSIC_destroy_cont_input (MUSIC_cont_input_port* port);
void MUSIC_destroy_event_output (MUSIC_event_output_port* port);
void MUSIC_destroy_event_input (MUSIC_event_input_port* port);
void MUSIC_destroy_message_output (MUSIC_message_output_port* port);
void MUSIC_destroy_message_input (MUSIC_message_input_port* port);

/* General port methods */

/* xxx = cont | event
   ddd = output | input */

int MUSIC_xxx_ddd_port_is_connected (xxx_ddd_port *port);
int MUSIC_message_ddd_port_is_connected (message_ddd_port *port);
int MUSIC_xxx_ddd_port_has_width (xxx_ddd_port *port);
int MUSIC_xxx_ddd_port_width (xxx_ddd_port *port);

/* Mapping */

/* No arguments are optional. */

void MUSIC_cont_output_port_map (MUSIC_cont_output_port *port,
				 MUSIC_cont_data *dmap,
				 int max_buffered);

void MUSIC_cont_input_port_map (MUSIC_cont_input_port *port,
				MUSIC_cont_data *dmap,
				double delay,
				int max_buffered,
				int interpolate);

void MUSIC_event_output_port_map (MUSIC_event_output_port *port,
				  MUSIC_index_map *indices,
				  int max_buffered);

typedef void MUSIC_event_hahndler (double t, int id);

void MUSIC_event_input_port_map (MUSIC_event_input_port *port,
				 MUSIC_index_map *indices,
				 MUSIC_event_handler *handle_event,
				 double acc_latency,
				 int max_buffered);

void MUSIC_message_output_port_map (MUSIC_message_output_port *port,
				    int max_buffered);

typedef void MUSIC_event_hahndler (double t, void *msg, size_t size);

void MUSIC_message_input_port_map (MUSIC_message_handler *handle_message,
				   double acc_latency,
				   int max_buffered);

/* Index maps */

MUSIC_permutation_index *MUSIC_create_permutation_index (int *indices,
							 int size);

void MUSIC_destroy_permutation_index (MUSIC_permutation_index *index);

MUSIC_linear_index *MUSIC_create_linear_index (int base_index,
					       int size);

void MUSIC_destroy_linear_index (MUSIC_linear_index *index);

/* Data maps */

/* Exception: The map argument can take any type of index map. */

MUSIC_array_data *MUSIC_create_array_data (void *buffer,
					   MPI_Datatype type,
					   void *map);

/* Exception: MUSIC_create_linear_array_data corresponds to
   c++ music::array_data::array_data (..., ..., ..., ...) */

MUSIC_array_data *MUSIC_create_linear_array_data (void *buffer,
						  MPI_Datatype type,
						  int base_index,
						  int size);

void MUSIC_destroy_array_data (MUSIC_array_data *array_data);

/* Configuration variables */

/* Exceptions: Result is char *
   Extra maxlen argument prevents buffer overflow.
   Result is terminated by \0 unless longer than maxlen - 1 */

int MUSIC_config (MUSIC_setup *setup,
		  char *name,
		  char *result,
		  size_t maxlen);

int MUSIC_config (MUSIC_setup *setup, char *name, int *result);

int MUSIC_config (MUSIC_setup *setup, char *name, double *result);

/* Runtime */

MUSIC_runtime *MUSIC_create_runtime (MUSIC_setup *setup, double h);

void MUSIC_tick (MUSIC_runtime *runtime);

double MUSIC_time (MUSIC_runtime *runtime);

/* Finalization */

void MUSIC_destroy_runtime (MUSIC_runtime *runtime);

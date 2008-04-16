/* Setup */

MUSIC_Setup *MUSIC_Create_setup (int *argc, char ***argv);

/* Communicators */

MPI_Intracomm MUSIC_Setup_communicator (MUSIC_Setup *setup);

/* Port creation */

MUSIC_Cont_output_port *MUSIC_Publish_cont_output (char *id);
MUSIC_Cont_input_port *MUSIC_Publish_cont_input (char *id);
MUSIC_Event_output_port *MUSIC_Publish_event_output (char *id);
MUSIC_Event_input_port *MUSIC_Publish_event_input (char *id);
MUSIC_Message_output_port *MUSIC_Publish_message_output (char *id);
MUSIC_Message_input_port *MUSIC_Publish_message_input (char *id);

void MUSIC_Destroy_cont_output (MUSIC_Cont_output_port* port);
void MUSIC_Destroy_cont_input (MUSIC_Cont_input_port* port);
void MUSIC_Destroy_event_output (MUSIC_Event_output_port* port);
void MUSIC_Destroy_event_input (MUSIC_Event_input_port* port);
void MUSIC_Destroy_message_output (MUSIC_Message_output_port* port);
void MUSIC_Destroy_message_input (MUSIC_Message_input_port* port);

/* General port methods */

/* XXX = ( Cont | Event | Message ) ( output | input ) */

int MUSIC_XXX_port_is_connected (XXX_port *port);
int MUSIC_XXX_port_has_width (XXX_port *port);
int MUSIC_XXX_port_width (XXX_port *port);

/* Mapping */

/* No arguments are optional. */

void MUSIC_Cont_output_port_map (MUSIC_Cont_output_port *port,
				 MUSIC_Cont_data *dmap,
				 int max_buffered);

void MUSIC_Cont_input_port_map (MUSIC_Cont_input_port *port,
				MUSIC_Cont_data *dmap,
				double delay,
				int max_buffered,
				int interpolate);

void MUSIC_Event_output_port_map (MUSIC_Event_output_port *port,
				  MUSIC_Index_map *indices,
				  int max_buffered);

typedef void MUSIC_Event_hahndler (double t, int id);

void MUSIC_Event_input_port_map (MUSIC_Event_input_port *port,
				 MUSIC_Index_map *indices,
				 MUSIC_Event_handler *handle_event,
				 double acc_latency,
				 int max_buffered);

void MUSIC_Message_output_port_map (MUSIC_Message_output_port *port,
				    int max_buffered);

typedef void MUSIC_Event_hahndler (double t, void *msg, size_t size);

void MUSIC_Message_input_port_map (MUSIC_Message_handler *handle_message,
				   double acc_latency,
				   int max_buffered);

/* Index maps */

MUSIC_Permutation_index *MUSIC_Create_permutation_index (int *indices,
							 int size);

void MUSIC_Destroy_permutation_index (MUSIC_Permutation_index *index);

MUSIC_Linear_index *MUSIC_Create_linear_index (int base_index,
					       int size);

void MUSIC_Destroy_linear_index (MUSIC_Linear_index *index);

/* Data maps */

/* Exception: The map argument can take any type of index map. */

MUSIC_Array_data *MUSIC_Create_array_data (void *buffer,
					   MPI_Datatype type,
					   void *map);

/* Exception: MUSIC_Create_linear_array_data corresponds to
   C++ MUSIC::array_data::array_data (..., ..., ..., ...) */

MUSIC_Array_data *MUSIC_Create_linear_array_data (void *buffer,
						  MPI_Datatype type,
						  int base_index,
						  int size);

void MUSIC_Destroy_array_data (MUSIC_Array_data *array_data);


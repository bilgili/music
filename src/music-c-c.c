#include <mpi.h>

/* Communicators */

MPI_Comm
MUSIC_setup_communicator (MUSIC_setup *setup)
{
  return (MPI_Comm) MUSIC_setup_communicator_glue (setup);
}

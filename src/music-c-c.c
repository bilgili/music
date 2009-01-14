#include <mpi.h>

#include "music-c.h"

/* Communicators */

MPI_Comm
MUSIC_setupCommunicator (MUSIC_Setup *setup)
{
  return (MPI_Comm) MUSIC_setupCommunicatorGlue (setup);
}

#include <mpi.h>

#include "music-c.h"

/* Communicators */

MPI_Comm
MUSICSetupCommunicator (MUSICSetup *setup)
{
  return (MPI_Comm) MUSICSetupCommunicatorGlue (setup);
}

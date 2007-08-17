#include <mpi.h>
#include "music.hh"


class MUSIC::runtime
{
  MPI_Comm
  MUSIC::communicator ()
  {
    return myCommunicator;
  }


  void
  MUSIC::finalize ()
  {
    MPI_Comm_free(&myCommunicator);
    MPI_Finalize();
  }


  void
  MUSIC::tick (double time)
  {
  }
}


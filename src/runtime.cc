#include <mpi.h>
#include "music/runtime.hh"


namespace MUSIC {
  
  MPI_Comm
  runtime::communicator ()
  {
    return myCommunicator;
  }


  void
  runtime::finalize ()
  {
    MPI_Comm_free (&myCommunicator);
    MPI_Finalize ();
  }


  void
  runtime::tick (double time)
  {
  }
}


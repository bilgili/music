#include <mpi.h>
#include "music/runtime.hh"


namespace MUSIC {
  
  MPI::Intercomm
  runtime::communicator ()
  {
    return myCommunicator;
  }


  void
  runtime::finalize ()
  {
    myCommunicator.Free ();
    MPI::Finalize ();
  }


  void
  runtime::tick (double time)
  {
  }
}


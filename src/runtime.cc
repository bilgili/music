#include <mpi.h>
#include "music/runtime.hh"


namespace MUSIC {
  
  MPI::Intracomm
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
    // Loop through the schedule of connectors
    std::vector<connector*>::iterator c;
    for (c = schedule->begin (); c != schedule->end (); ++c)
      c->tick ();
    
    local_time.tick ();
  }

}


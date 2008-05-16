#include <mpi.h>
#include "music/runtime.hh"


namespace MUSIC {

  runtime::runtime (setup* s, double h)
    //*fixme* 1e-9
    : local_time (1e-9, h)
  {
  }

  
  MPI::Intracomm
  runtime::communicator ()
  {
    return my_communicator;
  }


  void
  runtime::finalize ()
  {
    my_communicator.Free ();
    MPI::Finalize ();
  }


  void
  runtime::tick ()
  {
    // Loop through the schedule of connectors
    std::vector<connector*>::iterator c;
    for (c = schedule->begin (); c != schedule->end (); ++c)
      (*c)->tick ();
    
    local_time.tick ();
  }


  double
  runtime::time ()
  {
    return local_time.time ();
  }
}


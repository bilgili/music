#include <mpi.h>

namespace MUSIC
{

  setup
  init (int color, int* argc, char** argv[]);

  class setup
  {
    void
    declare (double* data, int count, char* name);

    void
    import (int peer, char* name);

    void
    export (int peer, char* name);

    runtime
    done ();
  };

  class runtime
  {
    MPI_Comm
    communicator ();

    void
    finalize ();

    void
    tick (double time);
  }

}

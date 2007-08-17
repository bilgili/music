#include <mpi.h>

namespace MUSIC
{

  class setup
  {
    void
    init (int color, int* argc, char** argv[]);

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

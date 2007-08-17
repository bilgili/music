#include <mpi.h>
#include "music.hh"

class MUSIC::setup
{
  private
  static MPI_Comm myCommunicator;

  public
  void
  MUSIC::init (int color, int* argc, char** argv[])
  {
    int myRank;

    MPI_Init(argc, argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
    MPI_Comm_split(MPI_COMM_WORLD, color, myRank, &myCommunicator);
  }


  MUSIC::runtime
  MUSIC::done ()
  {
    return new runtime()
  }
}

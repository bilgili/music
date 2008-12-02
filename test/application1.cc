// Application 1
// 
// This is a placeholder for a data generating application interfaced
// to Music



#include <mpi.h>
#include <music.hh>

#define DATA_SIZE 1000


double data[DATA_SIZE];


int
main (int nargs, char* argv[])
{
  double time;
  int rank;

  MUSIC::Setup* setup = new MUSIC::Setup (nargs, argv);

  MPI::Intracomm comm = setup->communicator ();
  
  rank = comm.Get_rank ();

  MUSIC::Runtime* runtime = new MUSIC::Runtime (setup, 0.1);

  for (time = 0.0; time < 1.0; time += 0.1) {
    if (rank == 0) {
      // Generate original data on master node
      int i;

      for (i = 0; i < DATA_SIZE; ++i)
	data[i] = 17.0;
    }

    comm.Scatter (data, DATA_SIZE, MPI_DOUBLE,
		  data, DATA_SIZE, MPI_DOUBLE,
		  0);

    runtime->tick ();
  }

  runtime->finalize ();

  return 0;
}

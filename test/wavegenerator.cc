// Wavegenerator
// 
// This is a placeholder for a data generating application interfaced
// to Music



#include <mpi.h>
#include <music.hh>
#include <math.h>

#define APPLICATION_ID 1
#define DATA_SIZE 1000


double data[DATA_SIZE];

int
main (int nargs, char* argv[])
{
  double time;
  int rank;

  MUSIC::setup* setup = new MUSIC::setup (APPLICATION_ID, &nargs, &argv);

  // Find out who we are
  MPI_Comm_rank (setup->communicator (), &rank);

  // Declare what data we have to export
  setup->publish (new MUSIC::array_data (data, MPI_DOUBLE,
					 new MUSIC::linear_index (DATA_SIZE,
								  DATA_SIZE*rank)),
		  "Wavedata");

  MUSIC::runtime* runtime = setup->done ();

  for (time = 0.0; time < 1.0; time += 0.1) {
    if (rank == 0) {
      // Generate original data on master node
      int i;

      for (i = 0; i < DATA_SIZE; ++i)
	data[i] = sin(time * i/1000.0);
    }

    // Broadcast these data out to all nodes
    MPI_Bcast (data, DATA_SIZE, MPI_DOUBLE,
	       0, runtime->communicator ());

    // Make data available for other programs
    runtime->tick (time);
  }

  runtime->finalize ();

  return 0;
}

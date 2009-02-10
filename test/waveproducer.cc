#include <mpi.h>
#include <cstdlib>
#include <cmath>
#include <music.hh>

#define TIMESTEP 0.001

MPI::Intracomm comm;
double* data;

int
main (int argc, char* argv[])
{
  int width = atoi (argv[1]); // command line arg gives width

  MUSIC::Setup* setup = new MUSIC::Setup (argc, argv);
  
  MUSIC::ContOutputPort* wavedata = setup->publishContOutput ("wavedata");

  comm = setup->communicator ();
  int nProcesses = comm.Get_size (); // how many processes are there?
  int rank = comm.Get_rank ();        // which process am I?
  // For clarity, assume that width is a multiple of n_processes
  int nLocalVars = width / nProcesses;
  data = new double[nLocalVars];
    
  // Declare what data we have to export
  MUSIC::ArrayData dmap (data,
			 MPI::DOUBLE,
			 rank * nLocalVars,
			 nLocalVars);
  wavedata->map (&dmap);
  
  double stoptime;
  setup->config ("stoptime", &stoptime);

  MUSIC::Runtime* runtime = new MUSIC::Runtime (setup, TIMESTEP);

  double time = runtime->time ();
  while (time < stoptime)
    {
      if (rank == 0)
	{
	  // Generate original data on master node
	  int i;

	  for (i = 0; i < nLocalVars; ++i)
	    data[i] = sin (2 * M_PI * time * i);
	}

      // Broadcast these data out to all nodes
      comm.Bcast (data, nLocalVars, MPI::DOUBLE, 0);

      // Make data available for other programs
      runtime->tick ();

      time = runtime->time ();
    }

  runtime->finalize ();

  delete runtime;

  return 0;
}

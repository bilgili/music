#include <mpi.h>
#include <cmath>
#include <music.hh>

#define TIMESTEP 0.001

MPI::Intracomm comm;
double* data;

int
main (int argc, char* argv[])
{
  int width = atoi (argv[1]); // command line arg gives width

  MUSIC::setup* setup = new MUSIC::setup (argc, argv);
  
  MUSIC::cont_output_port* wavedata = setup->publish_cont_output ("wavedata");

  comm = setup->communicator ();
  int n_processes = comm.Get_size (); // how many processes are there?
  int rank = comm.Get_rank ();        // which process am I?
  // For clarity, assume that width is a multiple of n_processes
  int n_local_vars = width / n_processes;
  data = new double[n_local_vars];
    
  // Declare what data we have to export
  MUSIC::array_data dmap (data,
			  MPI::DOUBLE,
			  rank * n_local_vars,
			  n_local_vars);
  wavedata->map (&dmap);
  
  double stoptime;
  setup->config ("stoptime", &stoptime);

  MUSIC::runtime* runtime = new MUSIC::runtime (setup, TIMESTEP);

  double time = runtime->time ();
  while (time < stoptime)
    {
      if (rank == 0)
	{
	  // Generate original data on master node
	  int i;

	  for (i = 0; i < n_local_vars; ++i)
	    data[i] = sin (2 * M_PI * time * i);
	}

      // Broadcast these data out to all nodes
      comm.Bcast (data, n_local_vars, MPI::DOUBLE, 0);

      // Make data available for other programs
      runtime->tick ();

      time = runtime->time ();
    }

  delete runtime;

  return 0;
}

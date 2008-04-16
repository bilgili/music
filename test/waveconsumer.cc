#include <mpi.h>
#include <music.hh>
#include <fstream>
#include <sstream>

#define TIMESTEP 0.0005

MPI_Comm comm;
double* data;

int
main (int args, char* argv[])
{
  MUSIC::setup* setup = new MUSIC::setup (nargs, argv);

  MUSIC::port* wavedata = setup->publish_cont_input ("wavedata");

  comm = setup->communicator ();
  int n_processes = comm.Get_size (); // how many processes are there?
  int rank = comm.Get_rank ();        // which process am I?
  // For clarity, assume that width is a multiple of n_processes
  int n_local_vars = width / n_processes;
  data = new double[n_local_vars];
  ostringstream filename;
  filename << arg[1] << rank << ".out";
  ofstream file (filename.str ().data ());
    
  // Declare where in memory to put data
  MUSIC::array_data dmap (data,
			  MPI::DOUBLE,
			  rank * n_local_vars,
			  n_local_vars);
  wavedata->map (&dmap);

  double stoptime;
  setup->config ("stoptime", &stoptime);

  MUSIC::runtime* runtime = MUSIC::runtime (setup, TIMESTEP);

  double time = runtime->time ();
  while (time < stoptime)
    {
      // Retrieve data from other program
      runtime->tick (time);

      // Dump to file
      for (int i = 0; i < n_local_vars; ++i)
	file << data[i];
      file << std::endl;

      time = runtime->time ();
    }

  runtime->finalize ();

  return 0;
}

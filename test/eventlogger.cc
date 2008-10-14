#include <mpi.h>
#include <cstdlib>
#include <iostream>
#include <music.hh>

#define TIMESTEP 0.010

MPI::Intracomm comm;

class my_event_handler: public MUSIC::event_handler_global_index {
  int rank;
public:
  my_event_handler (int _rank) : rank (_rank) { }
  void operator () (double t, MUSIC::global_index id)
  {
    // For now: just print out incoming events
    std::cout << "Rank " << rank
	      << ": Event " << id << " detected at " << t << std::endl;
  }
};


int
main (int args, char* argv[])
{
  MUSIC::setup* setup = new MUSIC::setup (args, argv);

  comm = setup->communicator ();
  int n_processes = comm.Get_size (); // how many processes are there?
  int rank = comm.Get_rank ();        // which process am I?

  // Port publishing
  MUSIC::event_input_port* evport = setup->publish_event_input ("eventlog");
  if (!evport->is_connected ())
    {
      if (rank == 0)
	std::cerr << "eventlog port is not connected" << std::endl;
      exit (1);
    }

  // Split the width among the available processes
  int width;
  if (evport->has_width ())
    width = evport->width ();
  else
    comm.Abort (1);

  int n_local = width / n_processes;
  int rest = width % n_processes;
  int first_id = n_local * rank;
  if (rank < rest)
    {
      first_id += rank;
      n_local += 1;
    }
  else
    first_id += rest;
    
  // Port mapping
  MUSIC::linear_index indexmap (first_id, n_local);
  my_event_handler evhandler (rank);

  evport->map (&indexmap, &evhandler, 0.0);

  double stoptime;
  setup->config ("stoptime", &stoptime);

  // Run
  MUSIC::runtime* runtime = new MUSIC::runtime (setup, TIMESTEP);

  double time = runtime->time ();
  while (time < stoptime)
    {
      // Retrieve data from other program
      runtime->tick ();

      time = runtime->time ();
    }

  runtime->finalize ();
  
  delete runtime;

  return 0;
}

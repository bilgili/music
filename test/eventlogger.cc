#include <cstdlib>
#include <iostream>
#include <mpi.h>
#include <music.hh>

#define TIMESTEP 0.010

MPI::Intracomm comm;

class my_event_handler: public MUSIC::event_handler {
  void operator () (double t, int id)
  {
    // For now: just print out incoming events
    std::cout << "Event " << id << " detected at " << t << std::endl;
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

  // Split the width among the available processes
  int width;
  if (evport->has_width ())
    width = evport->width ();
  else
    comm.Abort (1);

  int n_local = width / n_processes;
    
  // Port mapping
  MUSIC::linear_index indexmap (rank * n_local, n_local);
  my_event_handler evhandler;

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

#include <mpi.h>
#include <music.hh>
#include <iostream>

#define TIMESTEP 0.01

double data[1];

int
main (int args, char* argv[])
{
  MUSIC::Setup* setup = new MUSIC::Setup (args, argv);

  MUSIC::ContInputPort* contdata = setup->publishContInput ("contdata");

  // Declare where in memory to put data
  MUSIC::ArrayData dmap (data,
			 MPI::DOUBLE,
			 0,
			 1);
  contdata->map (&dmap);

  double stoptime;
  setup->config ("stoptime", &stoptime);

  MUSIC::Runtime* runtime = new MUSIC::Runtime (setup, TIMESTEP);

  double time = runtime->time ();
  while (time < stoptime)
    {
      // Retrieve data from other program
      runtime->tick ();

      std::cout << data[0] << " at " << runtime->time () << std::endl;
    }

  runtime->finalize ();
  
  delete runtime;

  return 0;
}

#include <mpi.h>
#include <music.hh>
#include <iostream>

#define TIMESTEP 0.01

double *data;

int
main (int args, char* argv[])
{
  MUSIC::Setup* setup = new MUSIC::Setup (args, argv);

  MUSIC::ContInputPort* contdata = setup->publishContInput ("contdata");

  int width = contdata->width ();
  data = new double[width];

  // Declare where in memory to put data
  MUSIC::ArrayData dmap (data,
			 MPI::DOUBLE,
			 0,
			 width);
  contdata->map (&dmap);

  double stoptime;
  setup->config ("stoptime", &stoptime);

  MUSIC::Runtime* runtime = new MUSIC::Runtime (setup, TIMESTEP);


  for (; runtime->time () < stoptime; runtime->tick ())
    {
      for (int i = 0; i < width; ++i)
	std::cout << data[i] << " ";
      std::cout << "at " << runtime->time () << std::endl;
    }

  runtime->finalize ();
  
  delete runtime;

  return 0;
}

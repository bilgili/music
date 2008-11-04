#include <mpi.h>
#include <cstdlib>
#include <iostream>
#include <music.hh>

extern "C" {
#include <unistd.h>
#include <getopt.h>
};

const double DEFAULT_TIMESTEP = 0.01;

MPI::Intracomm comm;

void
usage (int rank)
{
  if (rank == 0)
    {
      std::cerr << "Usage: eventlogger [OPTION...]" << std::endl
		<< "`eventlogger' logs spikes from a MUSIC port." << std::endl << std::endl
		<< "  -t, --timestep TIMESTEP time between tick() calls (default " << DEFAULT_TIMESTEP << " s)" << std::endl
		<< "  -m, --imaptype TYPE     linear (default) or roundrobin" << std::endl
		<< "  -i, --indextype TYPE    global (default) or local" << std::endl
		<< "  -h, --help              print this help message" << std::endl << std::endl
		<< "Report bugs to <mikael@djurfeldt.com>." << std::endl;
    }
  exit (1);
}

class my_event_handler_global : public MUSIC::event_handler_global_index {
  int rank;
public:
  my_event_handler_global (int _rank) : rank (_rank) { }
  void operator () (double t, MUSIC::global_index id)
  {
    // For now: just print out incoming events
    std::cout << "Rank " << rank
	      << ": Event " << id << " detected at " << t << std::endl;
  }
};

class my_event_handler_local: public MUSIC::event_handler_local_index {
  int rank;
public:
  my_event_handler_local (int _rank) : rank (_rank) { }
  void operator () (double t, MUSIC::local_index id)
  {
    // For now: just print out incoming events
    std::cout << "Rank " << rank
	      << ": Event " << id << " detected at " << t << std::endl;
  }
};

double timestep = DEFAULT_TIMESTEP;
std::string imaptype = "linear";
std::string indextype = "global";

int
main (int argc, char* argv[])
{
  MUSIC::setup* setup = new MUSIC::setup (argc, argv);
  comm = setup->communicator ();
  int n_processes = comm.Get_size (); // how many processes are there?
  int rank = comm.Get_rank ();        // which process am I?

  opterr = 0; // handle errors ourselves
  while (1)
    {
      static struct option long_options[] =
	{
	  {"timestep",  required_argument, 0, 't'},
	  {"imaptype",  required_argument, 0, 'm'},
	  {"indextype", required_argument, 0, 'i'},
	  {"help",      no_argument,       0, 'h'},
	  {0, 0, 0, 0}
	};
      /* `getopt_long' stores the option index here. */
      int option_index = 0;

      // the + below tells getopt_long not to reorder argv
      int c = getopt_long (argc, argv, "+t:m:i:h", long_options, &option_index);

      /* detect the end of the options */
      if (c == -1)
	break;

      switch (c)
	{
	case 't':
	  timestep = atof (optarg); //*fixme* error checking
	  continue;
	case 'm':
	  imaptype = optarg;
	  if (imaptype != "linear" && imaptype != "roundrobin")
	    {
	      usage (rank);
	      abort ();
	    }
	  continue;
	case 'i':
	  indextype = optarg;
	  if (indextype != "global" && indextype != "local")
	    {
	      usage (rank);
	      abort ();
	    }
	  continue;
	case '?':
	  break; // ignore unknown options
	case 'h':
	  usage (rank);

	default:
	  abort ();
	}
    }

  // Port publishing
  MUSIC::event_input_port* evport = setup->publish_event_input ("in");
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

  // Port mapping

  my_event_handler_global evhandler_global (rank);
  my_event_handler_local evhandler_local (rank);

  if (imaptype == "linear")
    {
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
      MUSIC::linear_index indexmap (first_id, n_local);
      
      if (indextype == "global")
	evport->map (&indexmap, &evhandler_global, 0.0);
      else
	evport->map (&indexmap, &evhandler_local, 0.0);
    }
  else
    {
      std::vector<MUSIC::global_index> v;
      for (int i = rank; i < width; i += n_processes)
	v.push_back (i);
      MUSIC::permutation_index indexmap (&v.front (), v.size ());
      
      if (indextype == "global")
	evport->map (&indexmap, &evhandler_global, 0.0);
      else
	evport->map (&indexmap, &evhandler_local, 0.0);
    }

  double stoptime;
  setup->config ("stoptime", &stoptime);

  // Run
  MUSIC::runtime* runtime = new MUSIC::runtime (setup, timestep);

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

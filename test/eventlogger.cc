#include <mpi.h>
#include <cstdlib>
#include <iostream>
#include <music.hh>

extern "C" {
#include <unistd.h>
#include <getopt.h>
}

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
		<< "  -l, --acclatency LATENCY acceptable data latency (s)" << std::endl
		<< "  -b, --maxbuffered TICKS maximal amount of data buffered" << std::endl
		<< "  -m, --imaptype TYPE     linear (default) or roundrobin" << std::endl
		<< "  -i, --indextype TYPE    global (default) or local" << std::endl
		<< "  -h, --help              print this help message" << std::endl << std::endl
		<< "Report bugs to <music-bugs@incf.org>." << std::endl;
    }
  exit (1);
}

double apptime;

class MyEventHandlerGlobal : public MUSIC::EventHandlerGlobalIndex {
  int rank;
public:
  MyEventHandlerGlobal (int rank_) : rank (rank_) { }
  void operator () (double t, MUSIC::GlobalIndex id)
  {
    // For now: just print out incoming events
    std::cout << "Rank " << rank
	      << ": Event (" << id << ", " << t
	      << ") detected at " << apptime << std::endl;
  }
};

class MyEventHandlerLocal: public MUSIC::EventHandlerLocalIndex {
  int rank;
public:
  MyEventHandlerLocal (int rank_) : rank (rank_) { }
  void operator () (double t, MUSIC::LocalIndex id)
  {
    // For now: just print out incoming events
    std::cout << "Rank " << rank
	      << ": Event (" << id << ", " << t
	      << ") detected at " << apptime << std::endl;
  }
};

double timestep = DEFAULT_TIMESTEP;
double latency = 0.0;
int    maxbuffered = 0;
std::string imaptype = "linear";
std::string indextype = "global";

int
main (int argc, char* argv[])
{
  MUSIC::Setup* setup = new MUSIC::Setup (argc, argv);
  comm = setup->communicator ();
  int nProcesses = comm.Get_size (); // how many processes are there?
  int rank = comm.Get_rank ();        // which process am I?

  opterr = 0; // handle errors ourselves
  while (1)
    {
      static struct option longOptions[] =
	{
	  {"timestep",  required_argument, 0, 't'},
	  {"acclatency", required_argument, 0, 'l'},
	  {"maxbuffered", required_argument, 0, 'b'},
	  {"imaptype",  required_argument, 0, 'm'},
	  {"indextype", required_argument, 0, 'i'},
	  {"help",      no_argument,       0, 'h'},
	  {0, 0, 0, 0}
	};
      /* `getopt_long' stores the option index here. */
      int optionIndex = 0;

      // the + below tells getopt_long not to reorder argv
      int c = getopt_long (argc, argv, "+t:l:b:m:i:h", longOptions, &optionIndex);

      /* detect the end of the options */
      if (c == -1)
	break;

      switch (c)
	{
	case 't':
	  timestep = atof (optarg); // NOTE: could add error checking
	  continue;
	case 'l':
	  latency = atof (optarg);
	  continue;
	case 'b':
	  maxbuffered = atoi (optarg);
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
  MUSIC::EventInputPort* evport = setup->publishEventInput ("in");
  if (!evport->isConnected ())
    {
      if (rank == 0)
	std::cerr << "port `in' is not connected" << std::endl;
      comm.Abort (1);
    }

  // Split the width among the available processes
  int width = 0;
  if (evport->hasWidth ())
    width = evport->width ();
  else
    {
      std::cerr << "port width not specified in Configuration file" << std::endl;
      comm.Abort (1);
    }

  // Port mapping

  MyEventHandlerGlobal evhandlerGlobal (rank);
  MyEventHandlerLocal evhandlerLocal (rank);

  if (imaptype == "linear")
    {
      int nLocal = width / nProcesses;
      int rest = width % nProcesses;
      int firstId = nLocal * rank;
      if (rank < rest)
	{
	  firstId += rank;
	  nLocal += 1;
	}
      else
	firstId += rest;
      MUSIC::LinearIndex indexmap (firstId, nLocal);
      
      if (indextype == "global")
	if (maxbuffered > 0)
	  evport->map (&indexmap, &evhandlerGlobal, latency, maxbuffered);
	else
	  evport->map (&indexmap, &evhandlerGlobal, latency);
      else
	if (maxbuffered > 0)
	  evport->map (&indexmap, &evhandlerLocal, latency, maxbuffered);
	else
	  evport->map (&indexmap, &evhandlerLocal, latency);
    }
  else
    {
      std::vector<MUSIC::GlobalIndex> v;
      for (int i = rank; i < width; i += nProcesses)
	v.push_back (i);
      MUSIC::PermutationIndex indexmap (&v.front (), v.size ());
      
      if (indextype == "global")
	if (maxbuffered > 0)
	  evport->map (&indexmap, &evhandlerGlobal, latency, maxbuffered);
	else
	  evport->map (&indexmap, &evhandlerGlobal, latency);
      else
	if (maxbuffered > 0)
	  evport->map (&indexmap, &evhandlerLocal, latency, maxbuffered);
	else
	  evport->map (&indexmap, &evhandlerLocal, latency);
    }

  double stoptime;
  setup->config ("stoptime", &stoptime);

  // Run
  MUSIC::Runtime* runtime = new MUSIC::Runtime (setup, timestep);

  apptime = runtime->time ();
  while (apptime < stoptime)
    {
      // Retrieve data from other program
      runtime->tick ();

      apptime = runtime->time ();
    }

  runtime->finalize ();
  
  delete runtime;

  return 0;
}

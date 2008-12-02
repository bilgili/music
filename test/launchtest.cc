#include <music.hh>

#include <iostream>

int
main (int argc, char *argv[])
{
  MUSIC::Setup* musicSetup = new MUSIC::Setup (argc, argv);
  MPI::Intracomm comm = musicSetup->communicator ();
  int rank = comm.Get_rank ();
  double param;
  musicSetup->config ("param", &param);
  std::cout << "rank=" << rank << ":param=" << param;
  for (int i = 0; i < argc; ++i)
    std::cout << ':' << argv[i];
  std::cout << std::endl;
  MPI_Finalize ();
  return 0;
}

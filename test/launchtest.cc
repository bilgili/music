#include <music.hh>

#include <iostream>

int
main (int argc, char *argv[])
{
  MUSIC::setup* music_setup = new MUSIC::setup (argc, argv);
  MPI::Intracomm comm = music_setup->communicator ();
  int rank = comm.Get_rank ();
  double param;
  music_setup->config ("param", &param);
  std::cout << "rank=" << rank << ":param=" << param;
  for (int i = 0; i < argc; ++i)
    std::cout << ':' << argv[i];
  std::cout << std::endl;
  MPI_Finalize ();
  return 0;
}

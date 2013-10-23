
#include <music.hh>
#include <iostream>
#include <iterator>

int main(int argc, char* argv[])
{
  MUSIC::Setup *music_setup = new MUSIC::Setup(argc, argv);

  MPI::Intracomm comm = music_setup->communicator();

  int rank, num_processes;
  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &num_processes);

  MUSIC::Runtime *music_runtime = new MUSIC::Runtime(music_setup, 10.0 * 1e-3);

  unsigned int send_buffer_size = 10;

  std::vector<unsigned int> send_buffer = std::vector<unsigned int>(send_buffer_size, rank);
  std::vector<unsigned int> recv_buffer = std::vector<unsigned int>(send_buffer_size * num_processes, 0);

  std::cout << rank << " before allgather, send_buffer=[";
  std::copy(send_buffer.begin(), send_buffer.end(), std::ostream_iterator<unsigned int>(std::cout, ", ") );
  std::cout << "]" << std::endl;

  std::cout << rank << " before allgather, recv_buffer=[";
  std::copy(recv_buffer.begin(), recv_buffer.end(), std::ostream_iterator<unsigned int>(std::cout, ", ") );
  std::cout << "]" << std::endl;

  int error;
  error = MPI_Allgather(&send_buffer[0], send_buffer_size, MPI_UNSIGNED, 
                        &recv_buffer[0], send_buffer_size, MPI_UNSIGNED, comm);

  std::cout << rank << " after allgather, error=" << error << std::endl;

  std::cout << rank << " after allgather, send_buffer=[";
  std::copy(send_buffer.begin(), send_buffer.end(), std::ostream_iterator<unsigned int>(std::cout, ", ") );
  std::cout << "]" << std::endl;

  std::cout << rank << " after allgather, recv_buffer=[";
  std::copy(recv_buffer.begin(), recv_buffer.end(), std::ostream_iterator<unsigned int>(std::cout, ", ") );
  std::cout << "]" << std::endl;

  music_runtime->finalize();
  delete music_runtime;

  return 0;
}

#include <mpi.h>

int main(int argc, char* argv[])
{
    int res = MPI::Init_thread(argc, argv, 0);
    (void)res;
}

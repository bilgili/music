#ifndef MUSIC_C_H
#define MUSIC_C_H

#include "mpi.h"
#include "music/setup.hh"
#include "music/runtime.hh"

inline MPI_Comm communicator(MUSIC::Setup* s) {
  return (MPI_Comm) s->communicator();
}

inline MPI_Comm communicator(MUSIC::Runtime* r) {
  return (MPI_Comm) r->communicator();
}

#endif // MUSIC_C_H

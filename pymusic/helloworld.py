#!/usr/bin/env python
"""
MUSIC Hello World
"""

import sys, music
setup = music.Setup (sys.argv)
from mpi4py import MPI

hwmess = "Hello, World! I am process %d of %d on %s.\n"
comm = setup.communicator ()
myrank = comm.Get_rank()
nprocs = comm.Get_size()
sys.stdout.write(hwmess % (myrank, nprocs))

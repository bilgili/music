#! /usr/bin/python

import music
from music import subrange

import sys
import numpy
from itertools import takewhile, dropwhile

setup = music.Setup()
stoptime = setup.config("stoptime", float)
timestep = setup.config("timestep", float)

comm = setup.comm
rank = comm.Get_rank()

pin = setup.publishContInput("in")
data = numpy.array([-2], dtype=numpy.int)
pin.map(data, base=rank)

runtime = music.Runtime(setup, timestep)
times = takewhile(lambda t: t < stoptime+timestep,
                  dropwhile(lambda t: t <timestep,
                            runtime))
for time in subrange(runtime, timestep, stoptime+timestep):
    sys.stdout.write("t={}\treceiver {}: received Hello from sender {}\n".format(time, rank, data[0]))

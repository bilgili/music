#! /usr/bin/python

import music
from music import tslice

import sys
import numpy

setup = music.Setup(sys.argv)
stoptime = setup.config("stoptime", float)
timestep = setup.config("timestep", float)

comm = setup.communicator()
rank = comm.Get_rank()

pin = setup.publishContInput("in")
data = numpy.array([-2], dtype=numpy.int)
dataMap = music.ArrayData(data, rank)
pin.map(dataMap)

runtime = music.Runtime(setup, timestep)
for time in tslice(runtime, timestep, stoptime+timestep):
    sys.stdout.write("t={}\treceiver {}: received Hello from sender {}\n".format(time, rank, data[0]))

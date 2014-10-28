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

pout = setup.publishContOutput("out")
data = numpy.array([-1], dtype=numpy.int)
dataMap = music.ArrayData(data, rank)
pout.map(dataMap)

runtime = music.Runtime(setup, timestep)
for time in tslice(runtime, stoptime):
    data[0] = rank
    sys.stdout.write("t={}\tsender {}: Hello!\n".format(time, rank))

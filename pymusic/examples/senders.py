#! /usr/bin/python

import music

import sys
import numpy
from itertools import takewhile

setup = music.Setup()
stoptime = setup.config("stoptime", float)
timestep = setup.config("timestep", float)

comm = setup.comm
rank = comm.Get_rank()

pout = setup.publishContOutput("out")
data = numpy.array([-1], dtype=numpy.int)
pout.map(data, base=rank)

runtime = music.Runtime(setup, timestep)
for time in takewhile(lambda t: t < stoptime, runtime):
    data[0] = rank
    sys.stdout.write("t={}\tsender {}: Hello!\n".format(time, rank))

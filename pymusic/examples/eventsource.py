#!/usr/bin/python

import music
from itertools import groupby, ifilter, takewhile
import sys

setup = music.Setup()
stoptime = setup.config("stoptime", float)
timestep = setup.config("timestep", float)
buf = setup.config("buffer", int)
events = setup.config("events", int)

comm = setup.comm
rank = comm.Get_rank()
size = comm.Get_size()

out = setup.publishEventOutput("out")

width = out.width()
local = width // size
rest = width % size
firstId = rank * local

# distribute the rest:
firstId += min(rank, rest)
if rank < rest: local += 1

out.map(music.Index.GLOBAL, base=firstId, size=local, 
        maxBuffered=buf)

runtime = music.Runtime(setup, timestep)

eventgen = ((firstId + i % local, i * stoptime / events)
            for i in xrange(events)) # index, time
steps = groupby(eventgen, lambda x: int(x[1]/timestep))

step, nextEvents = next(steps, (None, None))
times = ifilter(lambda t: int(t/timestep) == step,
                takewhile(lambda t: t < stoptime,
                          runtime))
for time in times:
    for index, when in nextEvents:
        sys.stderr.write("Insert rank {}: Event ({}, {}) at {}\n".
                         format(rank, index, when, time))
        out.insertEvent(when, index, music.Index.GLOBAL)
    step, nextEvents = next(steps, (None, None))

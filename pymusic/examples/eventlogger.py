#!/usr/bin/python

import music
import sys
from itertools import takewhile

setup = music.Setup()
stoptime = setup.config("stoptime", float)
timestep = setup.config("timestep", float)
buf = setup.config("buffer", int)
errorat = setup.config("errorat", int)
if errorat < 0: errorat = None

comm = setup.comm
rank = comm.Get_rank()
size = comm.Get_size()

inp = setup.publishEventInput("in")

width = inp.width()
local = width // size
rest = width % size
firstId = rank * local

# distribute the rest:
firstId += min(rank, rest)
if rank < rest: local += 1

def eventerr(d):
    if errorat is None: return
    if d >= errorat: raise RuntimeError("Hey")

time = -1
def eventfunc(d, t, i):
    eventerr(d)
    sys.stderr.write("Receive rank {}: Event ({}, {}) at {}\n".
                     format(rank, i, d, time))

inp.map(eventfunc, music.Index.GLOBAL, base=firstId, size=local,
        maxBuffered=buf)

runtime = music.Runtime(setup, timestep)
for time in takewhile(lambda t: t <= stoptime, runtime):
    pass

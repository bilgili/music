import os

from music import *

define ('np', 2)
define ('stoptime', 1.0)

application ('from', 'eventsource', '-b 1 10 spikes')
application ('to', 'eventlogger', '-b 2')

connect ('from.out', 'to.in', 10)

launch ()

#
#  This file is part of MUSIC.
#  Copyright (C) 2012 Mikael Djurfeldt
#
#  MUSIC is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 3 of the License, or
#  (at your option) any later version.
#
#  MUSIC is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

# This API allows for querying the MPI rank and specification of the
# information in MUSIC configuration files

import os

# This function now defined in predict_rank.py
#
#def predictRank ():
#    """
#    Returns the predicted MPI rank of this process
#    
#    :rtype: Integer
#    """
#    return 0

CONFIGVARNAME = '_MUSIC_CONFIG_'

# This function now defined in predict_rank.py
#
def postponeSetup ():
    """
    Postpones processing of configuration info until the creation of
    the first port.  Must be called before creation of MUSIC::Setup.
    """
    os.environ[CONFIGVARNAME] = 'POSTPONE'


class Application (object):
    def define (varName, value):
        """
        Define configuration variable varName to value value.
        """
        pass

    def connect (fromPort, toPort):
        """
        Connect fromPort to toPort.
        """
        pass


def define (varName, value):
    """
    Define configuration variable varName to value value.
    """
    pass


def application (name):
    """
    Return configuration object for application name.  Create it if it
    does not already exist.

    :rtype: Application
    """
    return lookupCreate (name)


def connect (fromPort, toPort):
    """
    Connect fromPort to toPort.
    """
    pass


def configure ():
    """
    Configure the MUSIC library using the information provided by
    define and connect.
    """
    pass

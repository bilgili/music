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

from music.predict_rank import predictRank

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

configDict = {}

applications = []

class Application (object):
    configDict = {}

    number = 0

    def __init__ (self, name):
        self.name = name
        Application.number += 1

    def __getitem__ (self, varName):
        if varName in self.configDict:
            return self.configDict[varname]
        return configDict[varname]
    
    def define (self, varName, value):
        """
        Define configuration variable varName to value value.
        """
        configDict[varName] = value

    def connect (self, fromPort, toPort, width):
        """
        Connect fromPort to toPort.
        """
        pass


def define (varName, value):
    """
    Define configuration variable varName to value value.
    """
    configDict[varName] = value


def application (name):
    """
    Return configuration object for application name.  Create it if it
    does not already exist.

    :rtype: Application
    """
    for app in applications:
        if app.name == name:
            return app
    app = Application (name)
    applications.append (app)
    return app


def connect (fromPort, toPort, width):
    """
    Connect fromPort to toPort specifying port width width.
    """
    pass


def configure ():
    """
    Configure the MUSIC library using the information provided by
    define and connect.
    """
    rank = predictRank ()

    app = applicationMap[rank]

    conf = app.name \
           + ':' + app.number \
           + ':' + applicationMap.conf () \
           + ':' + connectivityMap.conf ()

    configDict.update (app.configDict)

    for key in configDict:
        conf += ':' + key + ':' + configDict[key]

    os.environ[CONFIGVARNAME] = conf


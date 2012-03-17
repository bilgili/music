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

OUTPUT = '0'
INPUT = '1'

# This function now defined in predict_rank.py
#
def postponeSetup ():
    """
    Postpones processing of configuration info until the creation of
    the first port.  Must be called before creation of MUSIC::Setup.
    """
    os.environ[CONFIGVARNAME] = 'POSTPONE'


portCodes = {}
code = 0

def portCode (appName, portName):
    global code
    key = (appName, portName)
    if key in portCodes:
        return portCodes[key]
    portCodes[key] = code
    ans = code
    code += 1
    return ans


class ConnectivityMap (object):
    def __init__ (self):
        self.ports = {}
    
    def register (self, portName, direction, width, *connection):
        if portName in self.ports:
            (direction_, width_, connections) = self.ports[portName]
            #*fixme* error checks here
        else:
            connections = []
            self.ports[portName] = (direction, width, connections)
        connections.append (connection)

    def conf (self):
        conf = str (len (self.ports))
        for portName in self.ports:
            (direction, width, connections) = self.ports[portName]
            conf += ':' + portName + ':' + direction + ':' + width
            conf += ':' + str (len (connections))
            for connection in connections:
                for item in connection:
                    conf += ':' + item
        return conf


configDict = {}

class Application (object):
    def __init__ (self, name, number):
        self.name = name
        self.number = number
        self.configDict = {}
        self.connectivityMap = ConnectivityMap ()

    def __getitem__ (self, varName):
        if varName in self.configDict:
            return self.configDict[varName]
        return configDict[varName]
    
    def define (self, varName, value):
        """
        Define configuration variable varName to value value.
        """
        self.configDict[varName] = str (value)

    def connect (self, fromPort, toPort, width):
        """
        Connect fromPort to toPort.
        """
        connections.append ((self, fromPort, toPort, str (width)))


class ApplicationMap (object):
    def __init__ (self):
        self.applications = []
        self.appMap = {}
        self.appNumber = 0

    def __getitem__ (self, name):
        return self.appMap[name]

    def registerCreate (self, name):
        if name in self.appMap:
            return self.appMap[name]
        app = Application (name, self.appNumber)
        self.appNumber += 1
        self.applications.append (app)
        self.appMap[name] = app
        return app
    
    def rankLookup (self, rank):
        rankSum = 0
        for app in self.applications:
            rankSum += app.np
            if rank < rankSum:
                return app

    def conf (self):
        conf = str (len (self.applications))
        for app in self.applications:
            conf += ':' + app.name + ':' + app['np']
        return conf

applicationMap = ApplicationMap ()


def registerConnection (app, fromPort, toPort, width):
    if '.' in fromPort:
        fromAppName, fromPort = fromPort.split ('.')
        fromApp = applicationMap[fromAppName]
    elif not app:
        raise RuntimeError, 'incomplete port name: ' + fromPort
    else:
        fromApp = app
    if '.' in toPort:
        toAppName, toPort = toPort.split ('.')
        toApp = applicationMap[toAppName]
    elif not app:
        raise RuntimeError, 'incomplete port name: ' + toPort
    else:
        toApp = app
    fromApp.connectivityMap.register (fromPort, OUTPUT, width,
                                      toApp.name, toPort,
                                      str (portCode (toApp.name, toPort)),
                                      str (toApp.leader), str (toApp.np))
    toApp.connectivityMap.register (toPort, INPUT, width,
                                    toApp.name, toPort,
                                    str (portCode (toApp.name, toPort)),
                                    str (fromApp.leader), str (fromApp.np))


def define (varName, value):
    """
    Define configuration variable varName to value value.
    """
    configDict[varName] = str (value)


def application (name, binary = None, args = None):
    """
    Return configuration object for application name.  Create it if it
    does not already exist.

    :rtype: Application
    """
    app = applicationMap.registerCreate (name)
    if binary:
        app.define ('binary', binary)
    if args:
        app.define ('args', args)
    return app


connections = []

def connect (fromPort, toPort, width):
    """
    Connect fromPort to toPort specifying port width width.
    """
    connections.append ((False, fromPort, toPort, str (width)))


def configure ():
    """
    Configure the MUSIC library using the information provided by
    define and connect.
    """
    rankSum = 0
    for app in applicationMap.applications:
        app.np = int (app['np'])
        app.leader = rankSum
        rankSum += app.np

    for connection in connections:
        registerConnection (*connection)
    
    rank = predictRank ()
    app = applicationMap.rankLookup (rank)

    conf = app.name \
           + ':' + str (app.number) \
           + ':' + applicationMap.conf () \
           + ':' + app.connectivityMap.conf ()

    configDict.update (app.configDict)

    for key in configDict:
        conf += ':' + key + '=' + configDict[key]

    os.environ[CONFIGVARNAME] = conf

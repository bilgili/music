/*
 *  This file is part of MUSIC.
 *  Copyright (C) 2008, 2009 INCF
 *
 *  MUSIC is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  MUSIC is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

//#define MUSIC_DEBUG 1
#include <mpi.h>
#include <iostream>
#include "music/event_routingmap.hh"

namespace MUSIC {
void
OutputRoutingMap::insertRoutingInterval(EventRouter *router, IndexInterval i, FIBO *b)
{
	routingData.push_back(new OutputRoutingData(i,b));
	router->insertRoutingData(*(routingData.back()));

}
void
InputRoutingMap::insertRoutingInterval(EventRouter *router, IndexInterval i, EventHandlerGlobalIndex *h)
{
	routingData.push_back(new InputRoutingData(i,h));
	router->insertRoutingData(*(routingData.back()));
}

}

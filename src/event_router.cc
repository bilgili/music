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

#include <iostream>
#include "music/debug.hh"
#include "music/event.hh"
#include "music/event_router.hh"
namespace MUSIC {

void
TreeProcessingRouter::insertRoutingData (EventRoutingData &data)
{

	routingTable.add (data);
}


void
TreeProcessingRouter::buildTable ()
{
	MUSIC_LOG0 ("Routing table size for rank 0 = " << routingTable.size ());
	routingTable.build ();
}


void
TreeProcessingRouter::processEvent (double t, GlobalIndex id)
{

	Processor i (t, id);
	routingTable.search (id, &i);
}

void
TreeProcessingRouter::processEvent (double t, LocalIndex id)
{
	Processor i (t, id);
	routingTable.search (id, &i);
}
void
TableProcessingRouter::processEvent(double t, GlobalIndex id)
{
	int size = routingTable.size();
	for(int i = 0; i < size; ++i)
		if(id < routingTable[i].size()){
			if(routingTable[i][id] != NULL)
				routingTable[i][id]->process(t, id);
		}
}

void
TableProcessingRouter::insertRoutingData(EventRoutingData &data)
{

	std::map<void*,int>::iterator it;
	int size = indexer.size();
	int idx = size;
	it = indexer.find(data.Data());
	if(it != indexer.end() )
		idx = it->second;
	else
		indexer[data.Data()]= size;

	if(data.end() > routingTable[idx].size())
		routingTable[idx].resize(data.end());
	for(int i=data.begin(); i < data.end(); ++i)
		routingTable[idx][i] = &data;
}
TableProcessingRouter::~TableProcessingRouter()
{
	for(int i = 0; i < routingTable.size(); ++i)
		for(int j=0; j< routingTable[i].size(); ++j)
			if(routingTable[i][j] != NULL)
				delete routingTable[i][j];
}
}

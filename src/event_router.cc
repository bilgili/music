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
	std::map<void*, std::vector<EventRoutingData*> >::iterator it;
	for ( it=routingTable.begin() ; it != routingTable.end(); it++ )
		if(id < (int)((*it).second).size()){
			if(((*it).second)[id] != NULL)
				((*it).second)[id]->process(t, id);
		}
}
void
TableProcessingRouter::processEvent(double t, LocalIndex id)
{
	std::map<void*, std::vector<EventRoutingData*> >::iterator it;
	for ( it=routingTable.begin() ; it != routingTable.end(); it++ )
		if(id < (int)((*it).second).size()){
			if(((*it).second)[id] != NULL)
				((*it).second)[id]->process(t, id - ((*it).second)[id]->offset ());
		}
}

void
TableProcessingRouter::insertRoutingData(EventRoutingData &data)
{
	routingData.push_back(data.Clone());
	void *data_ = data.Data();
	//if it's additional index range for the existing Data
	//or it's a new Data with its new index range
	if(data.end() > (int)routingTable[data_].size())
		routingTable[data_].resize(data.end());
    // for each element i in the index range
	// data should be the same (buffer or event handler)
	for(int i=data.begin(); i < data.end(); ++i)
		routingTable[data_][i] = routingData.back();
}
TableProcessingRouter::~TableProcessingRouter(){
	std::vector<EventRoutingData*>::iterator it =routingData.begin();
	while(it != routingData.end()){delete (*it); it++;}
}
}

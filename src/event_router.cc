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
InputRoutingData::InputRoutingData(const IndexInterval &i,  IndexProcessor *specialized_processor):
				  EventRoutingData(i),
				  specialized_processor_(specialized_processor->clone())
{
}
InputRoutingData::InputRoutingData(const IndexInterval &i, EventHandlerPtr* h):EventRoutingData(i)
{
	  if(h->getType() == Index::GLOBAL)
		  specialized_processor_ = new GlobalIndexProcessor(h);
	  else
		  specialized_processor_ = new LocalIndexProcessor(h);
}
InputRoutingData::~InputRoutingData()
{
	delete specialized_processor_;
}
InputRoutingData:: InputRoutingData(const InputRoutingData& data)
{
	  specialized_processor_ = data.specialized_processor_->clone();
}
InputRoutingData &InputRoutingData::operator=(InputRoutingData &data){
		  delete specialized_processor_;
		  specialized_processor_ = data.specialized_processor_->clone();
		  return *this;
}
void*
InputRoutingData::Data()
{
	return specialized_processor_->getPtr();
}
void
InputRoutingData::process (double t, int id)
{
  		specialized_processor_->process(t, id);
}
EventRoutingData *
InputRoutingData::clone()const
{
	return new InputRoutingData(*this, specialized_processor_);
}
OutputRoutingData::OutputRoutingData(const IndexInterval &i, FIBO* b):EventRoutingData(i),buffer_ (b)
{

}
void *
OutputRoutingData::Data()
{
	return buffer_;
}
void
OutputRoutingData::process (double t, int id)
{
	Event* e = static_cast<Event*> (buffer_->insert ());
	e->t = t;
	e->id = id;
}
EventRoutingData *
OutputRoutingData::clone() const
{
	return new OutputRoutingData(*this, buffer_);
}

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
TreeProcessingRouter::processEvent (double t, int id)
{

	Processor i (t, id);
	routingTable.search (id, &i);
}

void
TableProcessingRouter::processEvent(double t, int id)
{
	std::vector<EventRoutingData*>::iterator it;
	if (routingTable.count(id)>0)
		for(it = routingTable[id].begin(); it != routingTable[id].end(); it++){
			(*it)->process(t, id - (*it)->offset());
		}
}

void
TableProcessingRouter::insertRoutingData(EventRoutingData &data)
{
	routingData.push_back(data.clone());
	for(int i=data.begin(); i < data.end(); ++i){
		//std::cerr << "insert:" <<  i  << ":" << data.offset() << std::endl;
		routingTable[i+data.offset()].push_back(routingData.back());
	}
}
TableProcessingRouter::~TableProcessingRouter(){
	std::vector<EventRoutingData*>::iterator it =routingData.begin();
	while(it != routingData.end()){delete (*it); it++;}
}
}

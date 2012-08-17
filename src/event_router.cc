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

  InputRoutingData::InputRoutingData (const IndexInterval &i,  IndexProcessor *specialized_processor):
    EventRoutingData(i),
    specialized_processor_ (specialized_processor->clone ())
  {
  }

  InputRoutingData::InputRoutingData (const IndexInterval &i, EventHandlerPtr* h) : EventRoutingData (i)
  {
    if(h->getType() == Index::GLOBAL)
      specialized_processor_ = new GlobalIndexProcessor(h);
    else
      specialized_processor_ = new LocalIndexProcessor(h);
  }

  InputRoutingData::~InputRoutingData ()
  {
    if (specialized_processor_ != NULL)
      delete specialized_processor_;
  }

  InputRoutingData:: InputRoutingData(const InputRoutingData& data)
    : EventRoutingData (data)
  {
    if (data.specialized_processor_ == NULL)
      specialized_processor_ = NULL;
    else
      specialized_processor_ = data.specialized_processor_->clone ();
  }

  InputRoutingData& InputRoutingData::operator= (const InputRoutingData& data)
  {
    EventRoutingData::operator= (data);
    if (data.specialized_processor_ == NULL)
      specialized_processor_ = NULL;
    else
      {
	delete specialized_processor_;
	specialized_processor_ = data.specialized_processor_->clone ();
      }
    return *this;
  }

  void
  InputRoutingData::process (double t, int id)
  {
    specialized_processor_->process(t, id);
  }

  OutputRoutingData::OutputRoutingData (const IndexInterval &i, FIBO* b) : EventRoutingData(i), buffer_ (b)
  {
  }

  void
  OutputRoutingData::process (double t, int id)
  {
    Event* e = static_cast<Event*> (buffer_->insert ());
    e->t = t;
    e->id = id;
  }

}

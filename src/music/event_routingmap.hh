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
#ifndef MUSIC_EVENT_ROUTINGMAP_HH

#include <map>
#include <vector>
#include <music/event_router.hh>
#include <music/FIBO.hh>
#include <music/interval_tree.hh>
#include <music/index_map.hh>
#include <music/event.hh>

namespace MUSIC {
template <class DataType>
class EventRoutingMap{
protected:
	std::vector<Interval>* intervals;
	std::vector<EventRoutingData*> routingData;
	typedef std::map<DataType, std::vector<IndexInterval> >DataMap;
	DataMap dataMap;
	EventRoutingMap () { intervals = new std::vector<Interval>; }
public:
	virtual ~EventRoutingMap ();
	void build(EventRouter *router);
	void insert (IndexInterval i, DataType data);
protected:
	void rebuildIntervals ();
	void fillRouter (EventRouter *router);
	virtual void insertRoutingInterval(EventRouter *router, IndexInterval i, DataType data)=0;

};
template<class DataType>
EventRoutingMap<DataType>::~EventRoutingMap (){
	delete intervals;
	for(int i=0; i < routingData.size(); ++i)
		delete routingData[i];
}
template<class DataType>
void
EventRoutingMap<DataType>::insert (IndexInterval i, DataType data)
{
	intervals->push_back (i);
	dataMap[data].push_back (i);
}

template<class DataType>
void
EventRoutingMap<DataType>::rebuildIntervals ()
{
	std::vector<Interval>* newIntervals = new std::vector<Interval>;

	// Sort all intervals
	sort (intervals->begin (), intervals->end ());

	// Build sequence of unions out of the original interval sequence
	std::vector<Interval>::iterator i = intervals->begin ();
	while (i != intervals->end ())
	{
		Interval current = *i++;

		while (i != intervals->end ()
				&& i->begin () <= current.end ())
		{
			// join intervals
			int maxEnd = std::max<int> (current.end (), i->end ());
			current.setEnd (maxEnd);
			++i;
		}
		newIntervals->push_back (current);
	}

	delete intervals;
	intervals = newIntervals;
}
template<class DataType>
void
EventRoutingMap<DataType>::build (EventRouter *router)
{
	fillRouter (router);
	router->buildTable();
}
template<class DataType>
void
EventRoutingMap<DataType>::fillRouter (EventRouter *router)
{
	rebuildIntervals ();

	typename DataMap::iterator pos;
	for (pos = dataMap.begin (); pos != dataMap.end (); ++pos)
	{
		sort (pos->second.begin (), pos->second.end ());

		std::vector<IndexInterval>::iterator i = pos->second.begin ();
		std::vector<Interval>::iterator mapped = intervals->begin ();
		while (i != pos->second.end ())
		{
			IndexInterval current = *i++;

			while (i != pos->second.end ()
					&& i->local () == current.local ())
			{
				// Define the gap between current and next interval
				int gapBegin = current.end ();
				int gapEnd = i->begin ();

				// Skip mapped intervals which end before gap
				while (mapped != intervals->end ()
						&& mapped->end () <= gapBegin)
					++mapped;

				// Check that gap does not overlap with any mapped interval
				if (mapped != intervals->end () && mapped->begin () < gapEnd)
					break;

				// Join intervals by closing over gap
				current.setEnd (i->end ());
				++i;
			}

			insertRoutingInterval (router, current, pos->first);
		}
	}
}

class InputRoutingMap:public EventRoutingMap<EventHandlerGlobalIndex*>
{
private:
	void insertRoutingInterval(EventRouter *router, IndexInterval i, EventHandlerGlobalIndex *h);
};
class OutputRoutingMap:public EventRoutingMap<FIBO*>
{
private:
	void insertRoutingInterval(EventRouter *router, IndexInterval i, FIBO *b);
};
}
#define MUSIC_EVENT_ROUTINGMAP_HH
#endif

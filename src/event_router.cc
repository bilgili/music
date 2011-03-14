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

//#define MUSIC_DEBUG
#include "music/debug.hh"

#include "music/event_router.hh"
#include "music/event.hh"

namespace MUSIC {

  void
  EventRouter::insertRoutingInterval (IndexInterval i, FIBO* b)
  {
    routingTable.add (EventRoutingData (i, b));
  }
  

  void
  EventRouter::buildTable ()
  {
    MUSIC_LOG0 ("Routing table size for rank 0 = " << routingTable.size ());
    routingTable.build ();
  }


  void
  EventRouter::insertEvent (double t, GlobalIndex id)
  {
    Inserter i (t, id);
    routingTable.search (id, &i);
  }

  void
  EventRouter::insertEvent (double t, LocalIndex id)
  {
    Inserter i (t, id);
    routingTable.search (id, &i);
  }

/*
 * remedius
 */
  void CommonEventRouter::adjustMaxWidth(int tbl_size){
	  if(tbl_size > routingTable.size())
		  for(int i=routingTable.size(); i < tbl_size; ++i){
			  routingTable.push_back(NULL);
		  }
  }
/*  void
  CommonEventRouter::newTable(){
	  current++;
	  IntervalTree<int, CommonEventRoutingData> newTbl;
	  routingTables.push_back(newTbl);
  }*/
  /*
   * remedius
   */
  void
  CommonEventRouter::insertRoutingInterval(IndexInterval itrvl, EventHandlerPtr *handleEvent){
	 // routingTables[current].add (CommonEventRoutingData(i,handleEvent->global()));
	  for(int i=itrvl.begin(); i < itrvl.end(); ++i){
		  if(routingTable[i] != NULL)
			  MUSIC_LOGR ("overlap indx: " << i);
		  routingTable[i] = handleEvent->global();
	  }

  }
  /*
   * remedius
   */
 void
 CommonEventRouter::buildTable(){
	 MUSIC_LOGR ("Routing table size = " << routingTables.size ());
/*	 for(int i =0; i < routingTables.size(); ++i )
		 routingTables[i].build ();*/
	 //routingTable.build ();
 }
 /*
  * remedius
  */
 void
 CommonEventRouter::processEvent( double t, GlobalIndex id){
	 EventHandler h(t, id);
/*	 for(int i =0; i < routingTables.size(); ++i ){
		 routingTables[i].search (id, &h);
	 }*/
	 if(id< routingTable.size() && routingTable[id] != NULL){
		 (*routingTable[id]) (t, id);
	 }
	 //routingTable.search (id, &h);
 }

  void
  EventRoutingMap::insert (IndexInterval i, FIBO* buffer)
  {
    intervals->push_back (i);
    bufferMap[buffer].push_back (i);
  }


  void
  EventRoutingMap::rebuildIntervals ()
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
  
  
  void
  EventRoutingMap::fillRouter (EventRouter& router)
  {
    rebuildIntervals ();
    
    BufferMap::iterator pos;
    for (pos = bufferMap.begin (); pos != bufferMap.end (); ++pos)
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
	    router.insertRoutingInterval (current, pos->first);
	  }
      }
  }
  
}

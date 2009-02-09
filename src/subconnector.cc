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
#include "music/debug.hh" // Must be included first on BG/L

#include "music/subconnector.hh"

namespace MUSIC {

  Subconnector::Subconnector (Synchronizer* _synch,
			      MPI::Intercomm _intercomm,
			      int remoteRank,
			      int receiverRank,
			      std::string receiverPortName)
    : synch (_synch),
      intercomm (_intercomm),
      _remoteRank (remoteRank),
      _receiverRank (receiverRank),
      _receiverPortName (receiverPortName)
  {
  }


  Subconnector::~Subconnector ()
  {
  }

  
  void
  Subconnector::connect ()
  {
#if 0
    std::cout << "Process " << MPI::COMM_WORLD.Get_rank () << " creating intercomm with local " << _localLeader << " and remote " << _remoteLeader << std::endl;
#endif
  }


  OutputSubconnector::OutputSubconnector (Synchronizer* synch,
					  MPI::Intercomm intercomm,
					  int remoteRank,
					  int receiverRank,
					  std::string receiverPortName,
					  int elementSize)
    : Subconnector (synch,
		    intercomm,
		    remoteRank,
		    receiverRank,
		    receiverPortName),
      _buffer (elementSize)
  {
  }

  
  void
  OutputSubconnector::send ()
  {
  }

  
  int
  OutputSubconnector::startIdx ()
  {
    return 0;
  }

  
  int
  OutputSubconnector::endIdx ()
  {
    return 0;
  }

  
  InputSubconnector::InputSubconnector ()
  {
    flushed = false;
  }


  void
  ContOutputSubconnector::mark ()
  {
  }


  EventOutputSubconnector::EventOutputSubconnector (Synchronizer* _synch,
						    MPI::Intercomm _intercomm,
						    int remoteRank,
						    std::string _receiverPortName)
    : Subconnector (_synch,
		    _intercomm,
		    remoteRank,
		    remoteRank,
		    _receiverPortName),
      OutputSubconnector (_synch,
			  _intercomm,
			  remoteRank,
			  remoteRank, // receiver_rank same as remote rank
			  _receiverPortName,
			  sizeof (Event))
  {
  }
  

  void
  EventOutputSubconnector::tick ()
  {
    if (synch->communicate ())
      send ();
  }


  void
  EventOutputSubconnector::send ()
  {
    void* data;
    int size;
    _buffer.nextBlock (data, size);
    //*fixme* marshalling
    char* buffer = static_cast <char*> (data);
    while (size >= SPIKE_BUFFER_MAX)
      {
	intercomm.Send (buffer,
			SPIKE_BUFFER_MAX,
			MPI::BYTE,
			_remoteRank,
			SPIKE_MSG);
	buffer += SPIKE_BUFFER_MAX;
	size -= SPIKE_BUFFER_MAX;
      }
    intercomm.Send (buffer, size, MPI::BYTE, _remoteRank, SPIKE_MSG);
  }

  
  void
  EventOutputSubconnector::flush (bool& dataStillFlowing)
  {
    if (!_buffer.isEmpty ())
      {
	MUSIC_LOGR ("sending data remaining in buffers");
	send ();
	dataStillFlowing = true;
      }
    else
      {
	Event* e = static_cast<Event*> (_buffer.insert ());
	e->id = FLUSH_MARK;
	send ();
      }
  }
  

  EventInputSubconnector::EventInputSubconnector (Synchronizer* synch,
						  MPI::Intercomm intercomm,
						  int remoteRank,
						  int receiverRank,
						  std::string receiverPortName)
    : Subconnector (synch,
		    intercomm,
		    remoteRank,
		    receiverRank,
		    receiverPortName),
      InputSubconnector ()
  {
  }


  EventInputSubconnectorGlobal::EventInputSubconnectorGlobal
  (Synchronizer* synch,
   MPI::Intercomm intercomm,
   int remoteRank,
   int receiverRank,
   std::string receiverPortName,
   EventHandlerGlobalIndex* eh)
    : Subconnector (synch,
		    intercomm,
		    remoteRank,
		    receiverRank,
		    receiverPortName),
      EventInputSubconnector (synch,
				intercomm,
				remoteRank,
				receiverRank,
				receiverPortName),
      handleEvent (eh)
  {
  }


  EventHandlerGlobalIndexDummy
  EventInputSubconnectorGlobal::dummyHandler;

  
  EventInputSubconnectorLocal::EventInputSubconnectorLocal
  (Synchronizer* synch,
   MPI::Intercomm intercomm,
   int remoteRank,
   int receiverRank,
   std::string receiverPortName,
   EventHandlerLocalIndex* eh)
    : Subconnector (synch,
		    intercomm,
		    remoteRank,
		    receiverRank,
		    receiverPortName),
      EventInputSubconnector (synch,
				intercomm,
				remoteRank,
				receiverRank,
				receiverPortName),
      handleEvent (eh)
  {
  }

  
  EventHandlerLocalIndexDummy
  EventInputSubconnectorLocal::dummyHandler;

  
  void
  EventInputSubconnector::tick ()
  {
    if (!flushed && synch->communicate ())
      receive ();    
  }


  //*fixme* isolate difference between global and local
  void
  EventInputSubconnectorGlobal::receive ()
  {
    char* data[SPIKE_BUFFER_MAX]; 
    MPI::Status status;
    int size;
    do
      {
	intercomm.Recv (data,
			SPIKE_BUFFER_MAX,
			MPI::BYTE,
			_remoteRank,
			SPIKE_MSG,
			status);
	Event* ev = (Event*) data;
	if (ev[0].id == FLUSH_MARK)
	  {
	    flushed = true;
	    MUSIC_LOGR ("received flush message");
	    return;
	  }
	size = status.Get_count (MPI::BYTE);
	int nEvents = size / sizeof (Event);
	MUSIC_LOGR ("received " << nEvents << "events");
	for (int i = 0; i < nEvents; ++i)
	  (*handleEvent) (ev[i].t, ev[i].id);
      }
    while (size == SPIKE_BUFFER_MAX);
  }


  void
  EventInputSubconnectorLocal::receive ()
  {
    char* data[SPIKE_BUFFER_MAX]; 
    MPI::Status status;
    int size;
    do
      {
	intercomm.Recv (data,
			SPIKE_BUFFER_MAX,
			MPI::BYTE,
			_remoteRank,
			SPIKE_MSG,
			status);
	Event* ev = (Event*) data;
	if (ev[0].id == FLUSH_MARK)
	  {
	    flushed = true;
	    return;
	  }
	size = status.Get_count (MPI::BYTE);
	int nEvents = size / sizeof (Event);
	for (int i = 0; i < nEvents; ++i)
	  (*handleEvent) (ev[i].t, ev[i].id);
      }
    while (size == SPIKE_BUFFER_MAX);
  }


  void
  EventInputSubconnector::flush (bool& dataStillFlowing)
  {
    if (!flushed)
      {
	MUSIC_LOGR ("receiving and throwing away data");
	receive ();
	if (!flushed)
	  dataStillFlowing = true;
      }
  }

  
  void
  EventInputSubconnectorGlobal::flush (bool& dataStillFlowing)
  {
    handleEvent = &dummyHandler;
    EventInputSubconnector::flush (dataStillFlowing);
  }

  
  void
  EventInputSubconnectorLocal::flush (bool& dataStillFlowing)
  {
    handleEvent = &dummyHandler;
    EventInputSubconnector::flush (dataStillFlowing);
  }
  
}

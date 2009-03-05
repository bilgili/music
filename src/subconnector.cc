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

#include "music/communication.hh"

#include "music/subconnector.hh"

namespace MUSIC {

  Subconnector::Subconnector (Synchronizer* synch_,
			      MPI::Intercomm intercomm_,
			      int remoteRank,
			      int receiverRank,
			      std::string receiverPortName)
    : synch (synch_),
      intercomm (intercomm_),
      remoteRank_ (remoteRank),
      receiverRank_ (receiverRank),
      receiverPortName_ (receiverPortName)
  {
  }


  Subconnector::~Subconnector ()
  {
  }

  
  OutputSubconnector::OutputSubconnector (int elementSize)
    : buffer_ (elementSize)
  {
  }

  
  InputSubconnector::InputSubconnector ()
  {
    flushed = false;
  }


  /********************************************************************
   *
   * Cont Subconnectors
   *
   ********************************************************************/

  ContOutputSubconnector::ContOutputSubconnector (Synchronizer* synch_,
						  MPI::Intercomm intercomm_,
						  int remoteRank,
						  std::string receiverPortName_)
    : Subconnector (synch_,
		    intercomm_,
		    remoteRank,
		    remoteRank,
		    receiverPortName_),
      OutputSubconnector (0)
  {
  }
  

  void
  ContOutputSubconnector::maybeCommunicate ()
  {
    if (synch->communicate ())
      send ();
  }


  void
  ContOutputSubconnector::send ()
  {
    void* data;
    int size;
    buffer_.nextBlock (data, size);
    // NOTE: marshalling
    char* buffer = static_cast <char*> (data);
    while (size >= CONT_BUFFER_MAX)
      {
	MUSIC_LOGR ("Sending to rank " << remoteRank_);
	intercomm.Send (buffer,
			CONT_BUFFER_MAX,
			MPI::BYTE,
			remoteRank_,
			CONT_MSG);
	buffer += CONT_BUFFER_MAX;
	size -= CONT_BUFFER_MAX;
      }
    intercomm.Send (buffer, size, MPI::BYTE, remoteRank_, CONT_MSG);
  }

  
  void
  ContOutputSubconnector::flush (bool& dataStillFlowing)
  {
    if (!buffer_.isEmpty ())
      {
	MUSIC_LOGR ("sending data remaining in buffers");
	send ();
	dataStillFlowing = true;
      }
    else
      {
	char dummy;
	intercomm.Send (&dummy, 0, MPI::BYTE, remoteRank_, FLUSH_MSG);
      }
  }
  

  ContInputSubconnector::ContInputSubconnector (Synchronizer* synch,
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


  void
  ContInputSubconnector::maybeCommunicate ()
  {
    if (!flushed && synch->communicate ())
      receive ();    
  }


  void
  ContInputSubconnector::receive ()
  {
    char* data = static_cast<char*> (buffer_.insertBlock ());
    MPI::Status status;
    int size;
    do
      {
	MUSIC_LOGR ("Receiving from rank " << remoteRank_);
	intercomm.Recv (data,
			CONT_BUFFER_MAX,
			MPI::BYTE,
			remoteRank_,
			MPI::ANY_TAG,
			status);
	if (status.Get_tag () == FLUSH_MSG)
	  {
	    flushed = true;
	    MUSIC_LOGR ("received flush message");
	    return;
	  }
	size = status.Get_count (MPI::BYTE);
	buffer_.trimBlock (size);
      }
    while (size == CONT_BUFFER_MAX);
  }


  void
  ContInputSubconnector::flush (bool& dataStillFlowing)
  {
    if (!flushed)
      {
	MUSIC_LOGR ("receiving and throwing away data");
	receive ();
	if (!flushed)
	  dataStillFlowing = true;
      }
  }

  
  /********************************************************************
   *
   * Event Subconnectors
   *
   ********************************************************************/


  EventOutputSubconnector::EventOutputSubconnector (Synchronizer* synch_,
						    MPI::Intercomm intercomm_,
						    int remoteRank,
						    std::string receiverPortName_)
    : Subconnector (synch_,
		    intercomm_,
		    remoteRank,
		    remoteRank,
		    receiverPortName_),
      OutputSubconnector (sizeof (Event))
  {
  }
  

  void
  EventOutputSubconnector::maybeCommunicate ()
  {
    if (synch->communicate ())
      send ();
  }


  void
  EventOutputSubconnector::send ()
  {
    void* data;
    int size;
    buffer_.nextBlock (data, size);
    // NOTE: marshalling
    char* buffer = static_cast <char*> (data);
    while (size >= SPIKE_BUFFER_MAX)
      {
	intercomm.Send (buffer,
			SPIKE_BUFFER_MAX,
			MPI::BYTE,
			remoteRank_,
			SPIKE_MSG);
	buffer += SPIKE_BUFFER_MAX;
	size -= SPIKE_BUFFER_MAX;
      }
    intercomm.Send (buffer, size, MPI::BYTE, remoteRank_, SPIKE_MSG);
  }

  
  void
  EventOutputSubconnector::flush (bool& dataStillFlowing)
  {
    if (!buffer_.isEmpty ())
      {
	MUSIC_LOGR ("sending data remaining in buffers");
	send ();
	dataStillFlowing = true;
      }
    else
      {
	Event* e = static_cast<Event*> (buffer_.insert ());
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
  EventInputSubconnector::maybeCommunicate ()
  {
    if (!flushed && synch->communicate ())
      receive ();    
  }


  // NOTE: isolate difference between global and local to avoid code repetition
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
			remoteRank_,
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
			remoteRank_,
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
  
  /********************************************************************
   *
   * Message Subconnectors
   *
   ********************************************************************/

  MessageOutputSubconnector::MessageOutputSubconnector (Synchronizer* synch_,
							MPI::Intercomm intercomm_,
							int remoteRank,
							std::string receiverPortName_)
    : Subconnector (synch_,
		    intercomm_,
		    remoteRank,
		    remoteRank,
		    receiverPortName_),
      OutputSubconnector (1)
  {
  }
  

  void
  MessageOutputSubconnector::maybeCommunicate ()
  {
    if (synch->communicate ())
      send ();
  }


  void
  MessageOutputSubconnector::send ()
  {
    void* data;
    int size;
    buffer_.nextBlock (data, size);
    // NOTE: marshalling
    char* buffer = static_cast <char*> (data);
    while (size >= MESSAGE_BUFFER_MAX)
      {
	intercomm.Send (buffer,
			MESSAGE_BUFFER_MAX,
			MPI::BYTE,
			remoteRank_,
			MESSAGE_MSG);
	buffer += MESSAGE_BUFFER_MAX;
	size -= MESSAGE_BUFFER_MAX;
      }
    intercomm.Send (buffer, size, MPI::BYTE, remoteRank_, MESSAGE_MSG);
  }

  
  void
  MessageOutputSubconnector::flush (bool& dataStillFlowing)
  {
    if (!buffer_.isEmpty ())
      {
	MUSIC_LOGR ("sending data remaining in buffers");
	send ();
	dataStillFlowing = true;
      }
    else
      {
	char dummy;
	intercomm.Send (&dummy, 0, MPI::BYTE, remoteRank_, FLUSH_MSG);	
      }
  }
  

  MessageInputSubconnector::MessageInputSubconnector (Synchronizer* synch,
						      MPI::Intercomm intercomm,
						      int remoteRank,
						      int receiverRank,
						      std::string receiverPortName,
						      MessageHandler* mh)
    : Subconnector (synch,
		    intercomm,
		    remoteRank,
		    receiverRank,
		    receiverPortName),
      handleMessage (mh)
  {
  }


  MessageHandlerDummy
  MessageInputSubconnector::dummyHandler;

  
  void
  MessageInputSubconnector::maybeCommunicate ()
  {
    if (!flushed && synch->communicate ())
      receive ();    
  }


  // NOTE: isolate difference between global and local to avoid code repetition
  void
  MessageInputSubconnector::receive ()
  {
    char* data[MESSAGE_BUFFER_MAX]; 
    MPI::Status status;
    int size;
    do
      {
	intercomm.Recv (data,
			MESSAGE_BUFFER_MAX,
			MPI::BYTE,
			remoteRank_,
			MPI::ANY_TAG,
			status);
	if (status.Get_tag () == FLUSH_MSG)
	  {
	    flushed = true;
	    MUSIC_LOGR ("received flush message");
	    return;
	  }
	size = status.Get_count (MPI::BYTE);
	int current = 0;
	while (current < size)
	  {
	    MessageHeader* header = static_cast<MessageHeader*>
	      (static_cast<void*> (&data[current]));
	    current += sizeof (MessageHeader);
	    (*handleMessage) (header->t (), &data[current], header->size ());
	    current += header->size ();
	  }
      }
    while (size == MESSAGE_BUFFER_MAX);
  }


  void
  MessageInputSubconnector::flush (bool& dataStillFlowing)
  {
    handleMessage = &dummyHandler;
    if (!flushed)
      {
	MUSIC_LOGR ("receiving and throwing away data");
	receive ();
	if (!flushed)
	  dataStillFlowing = true;
      }
  }
  
}

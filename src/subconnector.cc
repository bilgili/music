/*
 *  This file is part of MUSIC.
 *  Copyright (C) 2008, 2009, 2010 INCF
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

#define MUSIC_DEBUG
#include "music/debug.hh" // Must be included first on BG/L

#include "music/communication.hh"

#include "music/subconnector.hh"

#ifdef MUSIC_DEBUG
#include <cstdlib>
#endif

namespace MUSIC {

  Subconnector::Subconnector (Synchronizer* synch_,
			      MPI::Intercomm intercomm_,
			      int remoteLeader,
			      int remoteRank,
			      int receiverRank,
			      int receiverPortCode)
    : synch (synch_),
      intercomm (intercomm_),
      remoteRank_ (remoteRank),
      remoteWorldRank_ (remoteLeader + remoteRank),
      receiverRank_ (receiverRank),
      receiverPortCode_ (receiverPortCode)
  {
  }


  Subconnector::~Subconnector ()
  {
  }

  
  BufferingOutputSubconnector::BufferingOutputSubconnector (int elementSize)
    : buffer_ (elementSize)
  {
  }
  /*
   * remedius
   */
  CommonEventSubconnector::CommonEventSubconnector(int max_buf_size):BufferingOutputSubconnector(sizeof (Event)),
		  flushed(false),
		  max_buf_size_(max_buf_size){


  }
  /*
   * remedius
   */
  int CommonEventSubconnector::max_size = -1;
  /*
   * remediuds
   */
 void
 CommonEventSubconnector::add(std::vector<IndexInterval> intervals, EventHandlerPtr handleEvent, int port_width){
	 //router.newTable();
	 router.adjustMaxWidth(port_width);
	  std::vector<IndexInterval>::iterator i;
	  for( i = intervals.begin(); i != intervals.end(); ++i)
		  router.insertRoutingInterval(*i, &handleEvent);
  }
 /*
  * remediuds
  */
  void
  CommonEventSubconnector::build(){
	  router.buildTable();
  }
  /*
   * remedius
   */
  void CommonEventSubconnector::maybeCommunicate(){


	  if(max_buf_size_ > 0)
		  communicate1();
	  else
		  communicate2();
  }
  void CommonEventSubconnector::communicate1(){
	  void* data;
  	  int size, nProcesses;
  	  unsigned int dsize, pdsize;
  	  unsigned char* cur_buff, *recv_buff, *send_buff;
  	  if(flushed)
  		  return;
  	  unsigned int sEvent = sizeof(Event);

  	  MPI_Comm_size(MPI_COMM_WORLD,&nProcesses);

  	  buffer_.nextBlock (data, size);
  	  cur_buff = static_cast <unsigned char*> (data);

  	  //possible size of the data that is send by each of the process
  	  pdsize = max_buf_size_+4;
  	  //possible size of the data that is exchanged between all processes
  	  dsize = pdsize*nProcesses;
  	  /*
  	   * filling in sending buffer,
  	   * first 4 bytes are the size of the buffer
  	   */
  	  send_buff = new unsigned char[pdsize];
  	  memcpy(send_buff+4,cur_buff,size);
  	  for(int k = 0; k < 4 ; ++k){
  		  send_buff[k] = (unsigned char )(size & 0xff);
  		  size >>= 8;
  	  }
  	  //sending data
  	  recv_buff = new unsigned char[dsize];
  	  MPI_Allgather(send_buff, pdsize, MPI::BYTE, recv_buff, pdsize, MPI::BYTE, MPI_COMM_WORLD);
  	  //processing the data
  	  flushed = true;
  	  for(unsigned int i = 0; i < dsize; i+=max_buf_size_){
  		  /*
  		   * getting information about the size of the buffer,
  		   * that was received from each process.
  		   */
  		  unsigned int iSize = 0;
  		  unsigned char *cur_size = ( unsigned char*)(recv_buff+i);
  		  for(int k = 3; k >=0; --k)
  			  iSize =(iSize<<8) | cur_size[k];
  		  MUSIC_LOGR("size"<<iSize);
  		  /*
  		   * flushed flag controls that all processes finished their job
  		   * and sent FLUSH_MARK flag, otherwise processes should participate
  		   * in communication even if they send no data (iSize = 0)
  		   */
  		  if(iSize == 0)
  			  flushed = false;
  		  //data is stored in 4 bytes further
  		  i+=4;
  		  //processing the data
  		  for(unsigned int j = 0; j < iSize; j+=sEvent){
  			  Event* e = static_cast<Event*> ((void*)(recv_buff+i+j));
  			  if (e->id == FLUSH_MARK){
  				  break;
  			  }
  			  else{
  				  router.processEvent(e->t, e->id);
  				  flushed = false;
  			  }
  		  }
  	  }
  	  delete send_buff;
  	  delete recv_buff;
    }
 void CommonEventSubconnector::communicate2(){
	  void* data;
	  int size, nProcesses;
	  unsigned int dsize;
	  int* ppBytes, *displ;
	  char* cur_buff, *recv_buff;
	  if(flushed)
		  return;
	  unsigned int sEvent = sizeof(Event);

	  buffer_.nextBlock (data, size);
	  cur_buff = static_cast <char*> (data);

	  MPI_Comm_size(MPI_COMM_WORLD,&nProcesses);

	  ppBytes = new int[nProcesses];
	  //distributing the size of the buffer
	 // MUSIC_LOGN(0,size);
	  MPI_Allgather (&size, 1, MPI_INT, ppBytes, 1, MPI_INT, MPI_COMM_WORLD );
	 // MUSIC_LOGN(0,"after");
	  MPI_Barrier(MPI_COMM_WORLD);
	  //could it be that dsize is more then unsigned int?
	  dsize = 0;
	  displ = new int[nProcesses];
	  for(int i=0; i < nProcesses; ++i){
		  displ[i] = dsize;
		  dsize += ppBytes[i];
		  if(ppBytes[i] > max_size)
			  max_size = ppBytes[i];
	  }

	  recv_buff = new char[dsize];
	  //distributing the data
	  MPI_Allgatherv(cur_buff, size, MPI::BYTE, recv_buff, ppBytes, displ, MPI::BYTE, MPI_COMM_WORLD);
	  //processing the data

	  flushed = true;
	  for(unsigned int i = 0; i < dsize; i+=sEvent){
		  Event* e = static_cast<Event*> ((void*)(recv_buff+i));
		  if (e->id == FLUSH_MARK)
			  continue;
		  else{
			  router.processEvent(e->t, e->id);
			  flushed = false;

		  }
	  }
	  if(dsize/sEvent != nProcesses)
	  		 flushed = false;
	  delete ppBytes;
	  delete displ;
	  delete recv_buff;
  }

 /*
  * remedius
  */
 void CommonEventSubconnector::flush(bool &dataStillFlowing){
	 if (!buffer_.isEmpty ())
	 {
		 MUSIC_LOGR ("sending data remaining in buffers");
		 maybeCommunicate ();
		 dataStillFlowing = true;
	 }
	 else if (!flushed)
	 {
		 Event* e = static_cast<Event*> (buffer_.insert ());
		 e->id = FLUSH_MARK;
		// MUSIC_LOGR("sending FLUSH_MARK");
		 maybeCommunicate ();
		 if(!flushed)
			 dataStillFlowing = true;
	 }
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
						  int remoteLeader,
						  int remoteRank,
						  int receiverPortCode_,
						  MPI::Datatype type)
    : Subconnector (synch_,
		    intercomm_,
		    remoteLeader,
		    remoteRank,
		    remoteRank,
		    receiverPortCode_),
      BufferingOutputSubconnector (0),
      ContSubconnector (type)
  {
  }
  

  void
  ContOutputSubconnector::initialCommunication ()
  {
    send ();
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
			CONT_BUFFER_MAX / type_.Get_size (),
			type_,
			remoteRank_,
			CONT_MSG);
	buffer += CONT_BUFFER_MAX;
	size -= CONT_BUFFER_MAX;
      }
    MUSIC_LOGR ("Last send to rank " << remoteRank_);
    intercomm.Send (buffer,
		    size / type_.Get_size (),
		    type_,
		    remoteRank_,
		    CONT_MSG);
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
	intercomm.Send (&dummy, 0, type_, remoteRank_, FLUSH_MSG);
      }
  }
  

  ContInputSubconnector::ContInputSubconnector (Synchronizer* synch_,
						MPI::Intercomm intercomm,
						int remoteLeader,
						int remoteRank,
						int receiverRank,
						int receiverPortCode,
						MPI::Datatype type)
    : Subconnector (synch_,
		    intercomm,
		    remoteLeader,
		    remoteRank,
		    receiverRank,
		    receiverPortCode),
      InputSubconnector (),
      ContSubconnector (type)
  {
  }


  void
  ContInputSubconnector::initialCommunication ()
  {
    receive ();
    buffer_.fill (synch->initialBufferedTicks ());
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
    char* data;
    MPI::Status status;
    int size;
    do
      {
	data = static_cast<char*> (buffer_.insertBlock ());
	MUSIC_LOGR ("Receiving from rank " << remoteRank_);
	intercomm.Recv (data,
			CONT_BUFFER_MAX / type_.Get_size (),
			type_,
			remoteRank_,
			MPI::ANY_TAG,
			status);
	if (status.Get_tag () == FLUSH_MSG)
	  {
	    flushed = true;
	    MUSIC_LOGR ("received flush message");
	    return;
	  }
	size = status.Get_count (type_);
	buffer_.trimBlock (type_.Get_size () * size);
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
						    MPI::Intercomm intercomm,
						    int remoteLeader,
						    int remoteRank,
						    int receiverPortCode)
    : Subconnector (synch_,
		    intercomm,
		    remoteLeader,
		    remoteRank,
		    remoteRank,
		    receiverPortCode),
      BufferingOutputSubconnector (sizeof (Event))
  {
  }
  

  void
  EventOutputSubconnector::maybeCommunicate ()
  {
    if (synch->communicate ())
      send ();
    else
      MUSIC_LOGRE ("will not send");
  }


  void
  EventOutputSubconnector::send ()
  {
    MUSIC_LOGRE ("send");
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
  

  EventInputSubconnector::EventInputSubconnector (Synchronizer* synch_,
						  MPI::Intercomm intercomm,
						  int remoteLeader,
						  int remoteRank,
						  int receiverRank,
						  int receiverPortCode)
    : Subconnector (synch_,
		    intercomm,
		    remoteLeader,
		    remoteRank,
		    receiverRank,
		    receiverPortCode),
      InputSubconnector ()
  {
  }


  EventInputSubconnectorGlobal::EventInputSubconnectorGlobal
  (Synchronizer* synch_,
   MPI::Intercomm intercomm,
   int remoteLeader,
   int remoteRank,
   int receiverRank,
   int receiverPortCode,
   EventHandlerGlobalIndex* eh)
    : Subconnector (synch_,
		    intercomm,
		    remoteLeader,
		    remoteRank,
		    receiverRank,
		    receiverPortCode),
      EventInputSubconnector (synch_,
			      intercomm,
			      remoteLeader,
			      remoteRank,
			      receiverRank,
			      receiverPortCode),
      handleEvent (eh)
  {
  }


  EventHandlerGlobalIndexDummy
  EventInputSubconnectorGlobal::dummyHandler;

  
  EventInputSubconnectorLocal::EventInputSubconnectorLocal
  (Synchronizer* synch_,
   MPI::Intercomm intercomm,
   int remoteLeader,
   int remoteRank,
   int receiverRank,
   int receiverPortCode,
   EventHandlerLocalIndex* eh)
    : Subconnector (synch_,
		    intercomm,
		    remoteLeader,
		    remoteRank,
		    receiverRank,
		    receiverPortCode),
      EventInputSubconnector (synch_,
			      intercomm,
			      remoteLeader,
			      remoteRank,
			      receiverRank,
			      receiverPortCode),
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
    char data[SPIKE_BUFFER_MAX]; 
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
	    //MUSIC_LOGR ("received flush message");
	    return;
	  }
	size = status.Get_count (MPI::BYTE);
	int nEvents = size / sizeof (Event);
	//MUSIC_LOGR ("received " << nEvents << "events");
	for (int i = 0; i < nEvents; ++i)
	  (*handleEvent) (ev[i].t, ev[i].id);
      }
    while (size == SPIKE_BUFFER_MAX);
  }


  void
  EventInputSubconnectorLocal::receive ()
  {
    MUSIC_LOGRE ("receive");
    char data[SPIKE_BUFFER_MAX]; 
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
	MUSIC_LOGRE ("receiving and throwing away data");
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
							MPI::Intercomm intercomm,
							int remoteLeader,
							int remoteRank,
							int receiverPortCode,
							FIBO* buffer)
    : Subconnector (synch_,
		    intercomm,
		    remoteLeader,
		    remoteRank,
		    remoteRank,
		    receiverPortCode),
      buffer_ (buffer)
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
    buffer_->nextBlockNoClear (data, size);
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
    if (!buffer_->isEmpty ())
      {
	MUSIC_LOGRE ("sending data remaining in buffers");
	send ();
	dataStillFlowing = true;
      }
    else
      {
	char dummy;
	intercomm.Send (&dummy, 0, MPI::BYTE, remoteRank_, FLUSH_MSG);	
      }
  }
  

  MessageInputSubconnector::MessageInputSubconnector (Synchronizer* synch_,
						      MPI::Intercomm intercomm,
						      int remoteLeader,
						      int remoteRank,
						      int receiverRank,
						      int receiverPortCode,
						      MessageHandler* mh)
    : Subconnector (synch_,
		    intercomm,
		    remoteLeader,
		    remoteRank,
		    receiverRank,
		    receiverPortCode),
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


  void
  MessageInputSubconnector::receive ()
  {
    char data[MESSAGE_BUFFER_MAX]; 
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
	    MUSIC_LOGRE ("received flush message");
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
	MUSIC_LOGRE ("receiving and throwing away data");
	receive ();
	if (!flushed)
	  dataStillFlowing = true;
      }
  }
  
}

/*
 *  This file is part of MUSIC.
 *  Copyright (C) 2007, 2008, 2009 INCF
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

#ifndef MUSIC_SUBCONNECTOR_HH

#include <mpi.h>

#include <string>

#include <music/synchronizer.hh>
#include <music/FIBO.hh>
#include <music/BIFO.hh>
#include <music/event_router.hh>
#include <music/message.hh>

namespace MUSIC {

  // NOTE: Must be divisible by the size of the datatype of the data
  // maps passed to cont ports
  const int SPIKE_BUFFER_MAX = 10000 * sizeof (Event);
  const int CONT_BUFFER_MAX = SPIKE_BUFFER_MAX;
  const int MESSAGE_BUFFER_MAX = 10000;

  // The subconnector is responsible for the local side of the
  // communication between two MPI processes, one for each port of a
  // port pair.  It is created in connector::connect ().
  
  class Subconnector {
  private:
  protected:
    Synchronizer* synch;
    MPI::Intercomm intercomm;
    int remoteRank_;		// rank in inter-communicatir
    int remoteWorldRank_;	// rank in COMM_WORLD
    int receiverRank_;
    int receiverPortCode_;
  public:
    Subconnector () { }
    Subconnector (Synchronizer* synch,
		  MPI::Intercomm intercomm,
		  int remoteLeader,
		  int remoteRank,
		  int receiverRank,
		  int receiverPortCode);
    virtual ~Subconnector ();
    virtual void initialCommunication () { }
    virtual void maybeCommunicate () = 0;
    virtual void flush (bool& dataStillFlowing) = 0;
    int remoteRank () const { return remoteRank_; }
    int remoteWorldRank () const { return remoteWorldRank_; }
    int receiverRank () const { return receiverRank_; }
    int receiverPortCode () const { return receiverPortCode_; }
  };
  
  class OutputSubconnector : virtual public Subconnector {
  public:
    virtual FIBO* buffer () { return 0; }
  };
  
  class BufferingOutputSubconnector : virtual public OutputSubconnector {
  protected:
    FIBO buffer_;
  public:
    BufferingOutputSubconnector (int elementSize);
    FIBO* buffer () { return &buffer_; }
  };
  class EventSubconnector : virtual public Subconnector {
  protected:
    static const int FLUSH_MARK = -1;
  };
  /*
   * remedius
   */

  class CommonEventSubconnector:public BufferingOutputSubconnector,public EventSubconnector{
	  CommonEventRouter router;
	  bool flushed;
	  static int max_size;
	  int max_buf_size_;

  public:
	  CommonEventSubconnector(int max_buf_size = -1);
	  //CommonEventSubconnector(std::vector<IndexInterval> intervals, EventHandlerPtr handleEvent );
	  void maybeCommunicate ();
	  void build();
	  static bool wasMaxSizeCalc(){return max_size >= 0;}
	  static int getMaxSize(){return max_size;}
	  void add(std::vector<IndexInterval> intervals, EventHandlerPtr handleEvent, int port_width );
	  void flush (bool& dataStillFlowing);
  private:
	  void communicate1();
	  void communicate2();
  };

  class InputSubconnector : virtual public Subconnector {
  protected:
    InputSubconnector ();
    bool flushed;
  public:
    virtual BIFO* buffer () { return NULL; }
  };

  class ContSubconnector : virtual public Subconnector {
  protected:
    MPI::Datatype type_;
  public:
    ContSubconnector (MPI::Datatype type)
      : type_ (type) { };
  };
  
  class ContOutputSubconnector : public BufferingOutputSubconnector,
				 public ContSubconnector {
  public:
    ContOutputSubconnector (Synchronizer* synch,
			    MPI::Intercomm intercomm,
			    int remoteLeader,
			    int remoteRank,
			    int receiverPortCode,
			    MPI::Datatype type);
    void initialCommunication ();
    void maybeCommunicate ();
    void send ();
    void flush (bool& dataStillFlowing);
  };
  
  class ContInputSubconnector : public InputSubconnector,
				public ContSubconnector {
  protected:
    BIFO buffer_;
  public:
    ContInputSubconnector (Synchronizer* synch,
			   MPI::Intercomm intercomm,
			   int remoteLeader,
			   int remoteRank,
			   int receiverRank,
			   int receiverPortCode,
			   MPI::Datatype type);
    BIFO* buffer () { return &buffer_; }
    void initialCommunication ();
    void maybeCommunicate ();
    void receive ();
    void flush (bool& dataStillFlowing);
  };


  
  class EventOutputSubconnector : public BufferingOutputSubconnector,
				  public EventSubconnector {
  public:
    EventOutputSubconnector (Synchronizer* synch,
			     MPI::Intercomm intercomm,
			     int remoteLeader,
			     int remoteRank,
			     int receiverPortCode);
    void maybeCommunicate ();
    void send ();
    void flush (bool& dataStillFlowing);
  };
  
  class EventInputSubconnector : public InputSubconnector,
				 public EventSubconnector {
  public:
    EventInputSubconnector (Synchronizer* synch,
			    MPI::Intercomm intercomm,
			    int remoteLeader,
			    int remoteRank,
			    int receiverRank,
			    int receiverPortCode);
    void maybeCommunicate ();
    virtual void receive () = 0;
    virtual void flush (bool& dataStillFlowing);
  };

  class EventInputSubconnectorGlobal : public EventInputSubconnector {
    EventHandlerGlobalIndex* handleEvent;
    static EventHandlerGlobalIndexDummy dummyHandler;
  public:
    EventInputSubconnectorGlobal (Synchronizer* synch,
				  MPI::Intercomm intercomm,
				  int remoteLeader,
				  int remoteRank,
				  int receiverRank,
				  int receiverPortCode,
				  EventHandlerGlobalIndex* eh);
    void receive ();
    void flush (bool& dataStillFlowing);
  };

  class EventInputSubconnectorLocal : public EventInputSubconnector {
    EventHandlerLocalIndex* handleEvent;
    static EventHandlerLocalIndexDummy dummyHandler;
  public:
    EventInputSubconnectorLocal (Synchronizer* synch,
				 MPI::Intercomm intercomm,
				 int remoteLeader,
				 int remoteRank,
				 int receiverRank,
				 int receiverPortCode,
				 EventHandlerLocalIndex* eh);
    void receive ();
    void flush (bool& dataStillFlowing);
  };

  class MessageSubconnector : virtual public Subconnector {
  protected:
    static const int FLUSH_MARK = -1;
  };
  
  class MessageOutputSubconnector : public OutputSubconnector,
				  public MessageSubconnector {
    FIBO* buffer_;
  public:
    MessageOutputSubconnector (Synchronizer* synch,
			       MPI::Intercomm intercomm,
			       int remoteLeader,
			       int remoteRank,
			       int receiverPortCode,
			       FIBO* buffer);
    void maybeCommunicate ();
    void send ();
    void flush (bool& dataStillFlowing);
  };
  
  class MessageInputSubconnector : public InputSubconnector,
				   public MessageSubconnector {
    MessageHandler* handleMessage;
    static MessageHandlerDummy dummyHandler;
  public:
    MessageInputSubconnector (Synchronizer* synch,
			      MPI::Intercomm intercomm,
			      int remoteLeader,
			      int remoteRank,
			      int receiverRank,
			      int receiverPortCode,
			      MessageHandler* mh);
    void maybeCommunicate ();
    void receive ();
    void flush (bool& dataStillFlowing);
  };

}

#define MUSIC_SUBCONNECTOR_HH
#endif

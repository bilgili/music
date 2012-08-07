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
#include "music/debug.hh"
#if MUSIC_USE_MPI
#include <mpi.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <music/event_router.hh>
//#include <music/synchronizer.hh>
#include <music/FIBO.hh>
#include <music/BIFO.hh>
#include <music/event.hh>
#include <music/message.hh>

namespace MUSIC {

  // NOTE: Must be divisible by the size of the datatype of the data
  // maps passed to cont ports
  const int SPIKE_BUFFER_MAX = 10000 * sizeof (Event);
  const int CONT_BUFFER_MAX = SPIKE_BUFFER_MAX; //5000 of double values per input connector
  const int MESSAGE_BUFFER_MAX = 10000;

  // The subconnector is responsible for the local side of the
  // communication between two MPI processes, one for each port of a
  // port pair.  It is created in connector::connect ().
  
  class Subconnector {
  private:
  protected:
    //Synchronizer* synch;
	  MPI::Datatype type_;
    MPI::Intercomm intercomm;
    int remoteRank_;		// rank in inter-communicatir
    int remoteWorldRank_;	// rank in COMM_WORLD
    int receiverRank_;
    int receiverPortCode_;
    bool flushed;

  public:
    Subconnector ():type_(MPI::BYTE),flushed(false){};
    Subconnector (MPI::Datatype type): type_ (type) { }
    Subconnector (MPI::Datatype type,
		  MPI::Intercomm intercomm,
		  int remoteLeader,
		  int remoteRank,
		  int receiverRank,
		  int receiverPortCode);
    virtual ~Subconnector ();
    virtual void initialCommunication (double param) { }
    virtual void maybeCommunicate () = 0;
    virtual void flush (bool& dataStillFlowing) = 0;
    int remoteRank () const { return remoteRank_; }
    int remoteWorldRank () const { return remoteWorldRank_; }
    int receiverRank () const { return receiverRank_; }
    int receiverPortCode () const { return receiverPortCode_; }
  };
  
  class OutputSubconnector : virtual public Subconnector {
  protected:
	  OutputSubconnector (){};
  public:
    virtual FIBO* outputBuffer () { return NULL; }
  };
  
  class BufferingOutputSubconnector : virtual public OutputSubconnector {
  protected:
    FIBO buffer_;
  public:
    BufferingOutputSubconnector (int elementSize);
    FIBO* outputBuffer () { return &buffer_; }
  };

  class InputSubconnector : virtual public Subconnector {
  protected:
    InputSubconnector (){};
  public:
    virtual BIFO* inputBuffer () { return NULL; }
  };

  class ContOutputSubconnector : public BufferingOutputSubconnector{
  protected:
	  ContOutputSubconnector():BufferingOutputSubconnector (0){};
  public:
    ContOutputSubconnector (//Synchronizer* synch,
			    MPI::Intercomm intercomm,
			    int remoteLeader,
			    int remoteRank,
			    int receiverPortCode,
			    MPI::Datatype type);
    void initialCommunication (double param);
    void maybeCommunicate ();
    void send ();
    void flush (bool& dataStillFlowing);
  };
  
  class ContInputSubconnector : public InputSubconnector {
  protected:
    BIFO buffer_;
    ContInputSubconnector(){};
  public:
    ContInputSubconnector (//Synchronizer* synch,
			   MPI::Intercomm intercomm,
			   int remoteLeader,
			   int remoteRank,
			   int receiverRank,
			   int receiverPortCode,
			   MPI::Datatype type);
    BIFO* inputBuffer () { return &buffer_; }
    void initialCommunication (double initialBufferedTicks);
    void maybeCommunicate ();
    void receive ();
    void flush (bool& dataStillFlowing);
  };

  class EventSubconnector : virtual public Subconnector {
  protected:
    static const int FLUSH_MARK = -1;
  };
  


  class EventOutputSubconnector : public BufferingOutputSubconnector,
  				  public EventSubconnector {

  protected:
	  EventOutputSubconnector(): BufferingOutputSubconnector (sizeof (Event)){};
    public:

	  EventOutputSubconnector (//Synchronizer* synch,
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
   protected:
 	  EventInputSubconnector(){};
   public:
     EventInputSubconnector (//Synchronizer* synch,
 			    MPI::Intercomm intercomm,
 			    int remoteLeader,
 			    int remoteRank,
 			    int receiverRank,
 			    int receiverPortCode);
 	 void maybeCommunicate ();
     virtual void receive () {};
     virtual void flush (bool& dataStillFlowing);
   };

  class EventInputSubconnectorGlobal : public EventInputSubconnector {
    EventHandlerGlobalIndex* handleEvent;
  //  static EventHandlerGlobalIndexDummy dummyHandler;

  public:
    EventInputSubconnectorGlobal (//Synchronizer* synch,
				  MPI::Intercomm intercomm,
				  int remoteLeader,
				  int remoteRank,
				  int receiverRank,
				  int receiverPortCode,
				  EventHandlerGlobalIndex* eh);
    void receive ();
  //  void flush (bool& dataStillFlowing);
  };

  class EventInputSubconnectorLocal : public EventInputSubconnector {
    EventHandlerLocalIndex* handleEvent;
 //   static EventHandlerLocalIndexDummy dummyHandler;
  public:
    EventInputSubconnectorLocal (//Synchronizer* synch,
				 MPI::Intercomm intercomm,
				 int remoteLeader,
				 int remoteRank,
				 int receiverRank,
				 int receiverPortCode,
				 EventHandlerLocalIndex* eh);
    void receive ();
  //  void flush (bool& dataStillFlowing);
  };

  class MessageSubconnector : virtual public Subconnector {
  protected:
    static const int FLUSH_MARK = -1;
  };
  
  class MessageOutputSubconnector : public OutputSubconnector,
				  public MessageSubconnector {
    FIBO* buffer_;
  public:
    MessageOutputSubconnector (//Synchronizer* synch,
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
  //  static MessageHandlerDummy dummyHandler;
  public:
    MessageInputSubconnector (//Synchronizer* synch,
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
  /* remedius
     * CollectiveSubconnector class is used for collective communication
     * based on MPI::ALLGATHER function.
     */
    class CollectiveSubconnector: public virtual Subconnector
    {
    	int nProcesses, *ppBytes, *displ;
    protected:
  	  MPI::Intracomm intracomm_;

    protected:
  	  virtual ~CollectiveSubconnector();
  	  CollectiveSubconnector(MPI::Intracomm intracomm);
  	  void maybeCommunicate ();
  	  int calcCommDataSize(int local_data_size);
  	  std::vector<char> getCommData(char *cur_buff, int size);
  	  const int* getDisplArr(){return displ;}
  	  const int* getSizeArr(){return ppBytes;}
  	  virtual void communicate() = 0;
    };

    class EventCollectiveSubconnector: public EventInputSubconnector, public EventOutputSubconnector,  public CollectiveSubconnector{
    protected:
    	EventRouter *router_;
    public:
    	EventCollectiveSubconnector( MPI::Intracomm intracomm,  EventRouter *router):
    		CollectiveSubconnector (intracomm),
    		router_(router){};
    	void flush (bool& dataStillFlowing);
    	void maybeCommunicate ();
    private:
    	void communicate();
    };
    class ContCollectiveSubconnector: public ContInputSubconnector, public ContOutputSubconnector,  public CollectiveSubconnector{
    	std::multimap< int, Interval> intervals_; //the data we want
    	std::map<int,int> blockSizePerRank_; //contains mapping of rank->size of the communication data
    										//(size of communication data/nBuffered per rank is constant).
    	int width_; //port width in bytes
    public:
    	ContCollectiveSubconnector (std::multimap<int, Interval > intervals, int width, MPI::Intracomm intracomm,  MPI::Datatype type):
    		Subconnector(type),
    		CollectiveSubconnector (intracomm),
    		intervals_(intervals),
    		width_(width*type.Get_size ()){}
    	void initialCommunication (double initialBufferedTicks);
    	void maybeCommunicate ();
    	void flush (bool& dataStillFlowing);
    private:
    	void communicate();
    	void fillBlockSizes();
    };

}
#endif
#define MUSIC_SUBCONNECTOR_HH
#endif

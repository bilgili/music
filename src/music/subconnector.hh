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
#include <music/event.hh>

namespace MUSIC {

  const int SPIKE_MSG = 1;
  const int SPIKE_BUFFER_MAX = 10000 * sizeof (Event);

  // The subconnector is responsible for the local side of the
  // communication between two MPI processes, one for each port of a
  // port pair.  It is created in connector::connect ().
  
  class Subconnector {
  private:
  protected:
    Synchronizer* synch;
    MPI::Intercomm intercomm;
    int remoteRank_;
    int receiverRank_;
    std::string receiverPortName_;
  public:
    Subconnector () { }
    Subconnector (Synchronizer* synch,
		  MPI::Intercomm intercomm,
		  int remoteRank,
		  int receiverRank,
		  std::string receiverPortName);
    virtual ~Subconnector ();
    virtual void tick () = 0;
    virtual void flush (bool& dataStillFlowing) = 0;
    int remoteRank () const { return remoteRank_; }
    int receiverRank () const { return receiverRank_; }
    std::string receiverPortName () const { return receiverPortName_; }
    void connect ();
  };
  
  class OutputSubconnector : virtual public Subconnector {
  protected:
    FIBO buffer_;
  public:
    OutputSubconnector (Synchronizer* synch,
			MPI::Intercomm intercomm,
			int remoteRank,
			int receiverRank,
			std::string receiverPortName,
			int elementSize);
    FIBO* buffer () { return &buffer_; }
    void send ();
  };
  
  class InputSubconnector : virtual public Subconnector {
  protected:
    InputSubconnector ();
    bool flushed;
  public:
    virtual BIFO* buffer () { return NULL; }
  };

  class ContOutputSubconnector : public OutputSubconnector {
  public:
    void mark ();
  };
  
  class ContInputSubconnector : public InputSubconnector {
  protected:
    BIFO buffer_;
  public:
    BIFO* buffer () { return &buffer_; }
  };

  class EventSubconnector : virtual public Subconnector {
  protected:
    static const int FLUSH_MARK = -1;
  };
  
  class EventOutputSubconnector : public OutputSubconnector,
				  public EventSubconnector {
  public:
    EventOutputSubconnector (Synchronizer* synch_,
			     MPI::Intercomm intercomm_,
			     int remoteRank,
			     std::string receiverPortName_);
    void tick ();
    void send ();
    void flush (bool& dataStillFlowing);
  };
  
  class EventInputSubconnector : public InputSubconnector,
				 public EventSubconnector {
  public:
    EventInputSubconnector (Synchronizer* synch,
			    MPI::Intercomm intercomm,
			    int remoteRank,
			    int receiverRank,
			    std::string receiverPortName);
    void tick ();
    virtual void receive () = 0;
    virtual void flush (bool& dataStillFlowing);
  };

  class EventInputSubconnectorGlobal : public EventInputSubconnector {
    EventHandlerGlobalIndex* handleEvent;
    static EventHandlerGlobalIndexDummy dummyHandler;
  public:
    EventInputSubconnectorGlobal (Synchronizer* synch,
				  MPI::Intercomm intercomm,
				  int remoteRank,
				  int receiverRank,
				  std::string receiverPortName,
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
				 int remoteRank,
				 int receiverRank,
				 std::string receiverPortName,
				 EventHandlerLocalIndex* eh);
    void receive ();
    void flush (bool& dataStillFlowing);
  };

}

#define MUSIC_SUBCONNECTOR_HH
#endif

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

#ifndef MUSIC_NEGOTIATOR_HH

#include <mpi.h>
#include <vector>
#include <memory>

#include <music/index_map.hh>

namespace MUSIC {

  const int TRANSMITTED_INTERVALS_MAX = 10000;

  class SpatialNegotiationData {
    IndexInterval _interval;
    int _rank;
  public:
    SpatialNegotiationData () { }
    SpatialNegotiationData (IndexInterval i, int r)
      : _interval (i), _rank (r) { }
    SpatialNegotiationData (int b, int e, int l, int r)
      : _interval (b, e, l), _rank (r) { }
    const IndexInterval& interval () const { return _interval; }
    int begin () const { return _interval.begin (); }
    int end () const { return _interval.end (); }
    int local () const { return _interval.local (); }
    void setLocal (int l) { _interval.setLocal (l); }
    int rank () const { return _rank; }
  };


  typedef std::vector<SpatialNegotiationData> NegotiationIntervals;
  

  class NegotiationIterator {
  public:
    class Implementation {
    public:
      virtual bool end () = 0;
      virtual void operator++ () = 0;
      virtual SpatialNegotiationData* dereference () = 0;
      virtual Implementation* copy () = 0;
    };

    class IntervalTraversal : public Implementation {
      NegotiationIntervals& buffer;
      int interval;
    public:
      IntervalTraversal (NegotiationIntervals& _buffer)
	: buffer (_buffer), interval (0) { }
      bool end () { return interval == buffer.size (); }
      void operator++ () { ++interval; }
      SpatialNegotiationData* dereference () { return &buffer[interval]; }
      virtual Implementation* copy () { return new IntervalTraversal (*this); }
    };

    class BufferTraversal : public Implementation {
      std::vector<NegotiationIntervals>& buffers;
      int buffer;
      int interval;
      void findInterval ();
    public:
      BufferTraversal (std::vector<NegotiationIntervals>& buffers);
      bool end ();
      void operator++ ();
      SpatialNegotiationData* dereference ();
      virtual Implementation* copy () { return new BufferTraversal (*this); }
    };

  private:
    Implementation* _implementation;
  public:
    NegotiationIterator (Implementation* impl);
    NegotiationIterator (NegotiationIntervals& buffer);
    NegotiationIterator (std::vector<NegotiationIntervals>& buffers);
    ~NegotiationIterator ()
    {
      delete _implementation;
    }
    NegotiationIterator (const NegotiationIterator& i)
      : _implementation (i._implementation->copy ())
    {
    }
    const NegotiationIterator& operator= (const NegotiationIterator& i)
    {
      delete _implementation;
      _implementation = i._implementation->copy ();
      return *this;
    }
    bool end () { return _implementation->end (); }
    NegotiationIterator& operator++ ()
    {
      ++*_implementation;
      return *this;
    };
    SpatialNegotiationData* operator-> ()
    {
      return _implementation->dereference ();
    }
  };
  

  // The SpatialNegotiator negotiates with the remote application how
  // to redistribute data over a port pair.  This is done using the
  // algorithm described in Djurfeldt and Ekeberg (2009) with memory
  // complexity O (N / P) where N is the port width and P is the
  // number of MPI processes.

  class SpatialNegotiator {
  protected:
    MPI::Intracomm comm;
    IndexMap* indices;
    Index::Type type;
    std::vector<NegotiationIntervals> remote;
    int width;
    int maxLocalWidth_;
    int localRank;
    int nProcesses;
  public:
    SpatialNegotiator (IndexMap* indices, Index::Type type);
    void negotiateWidth ();
    int maxLocalWidth () { return maxLocalWidth_; }
    NegotiationIterator wrapIntervals (IndexMap::iterator beg,
				       IndexMap::iterator end,
				       Index::Type type,
				       int rank);
    void send (MPI::Comm& comm, int destRank,
	       NegotiationIntervals& intervals);
    void receive (MPI::Comm& comm, int sourceRank,
		  NegotiationIntervals& intervals);
    void allToAll (std::vector<NegotiationIntervals>& out,
		   std::vector<NegotiationIntervals>& in);
    NegotiationIterator canonicalDistribution (int width, int nProcesses);
    void intersectToBuffers (std::vector<NegotiationIntervals>& source,
			     std::vector<NegotiationIntervals>& dest,
			     std::vector<NegotiationIntervals>& buffers);
    void intersectToBuffers (NegotiationIterator source,
			       NegotiationIterator dest,
			       std::vector<NegotiationIntervals>& buffers);
  private:
    void intersectToBuffers2 (NegotiationIterator source,
			      NegotiationIterator dest,
			      std::vector<NegotiationIntervals>& buffers);
  public:
    virtual NegotiationIterator negotiate (MPI::Intracomm comm,
					   MPI::Intercomm intercomm,
					   int remoteNProc) = 0;
  };


  class SpatialOutputNegotiator : public SpatialNegotiator {
    std::vector<NegotiationIntervals> local;
    std::vector<NegotiationIntervals> results;
  public:
    SpatialOutputNegotiator (IndexMap* indices, Index::Type type)
      : SpatialNegotiator (indices, type) { }
    void negotiateWidth (MPI::Intercomm c);
    NegotiationIterator negotiate (MPI::Intracomm comm,
				   MPI::Intercomm intercomm,
				   int remoteNProc);
  };

  
  class SpatialInputNegotiator : public SpatialNegotiator {
  public:
    SpatialInputNegotiator (IndexMap* indices, Index::Type type)
      : SpatialNegotiator (indices, type) { }
    void negotiateWidth (MPI::Intercomm c);
    NegotiationIterator negotiate (MPI::Intracomm comm,
				   MPI::Intercomm intercomm,
				   int remoteNProc);
  };

}

#define MUSIC_NEGOTIATOR_HH
#endif

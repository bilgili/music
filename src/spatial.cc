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

#include "music/spatial.hh" // Must be included first on BG/L

#include <sstream>

#include "music/debug.hh"
#include "music/error.hh"

namespace MUSIC {

  NegotiationIterator::BufferTraversal::BufferTraversal
  (std::vector<NegotiationIntervals>& buffers_)
    : buffers (buffers_), buffer (0), interval (0)
  {
    findInterval ();
  }


  void
  NegotiationIterator::BufferTraversal::findInterval ()
  {
    while (buffer < buffers.size () && interval == buffers[buffer].size ())
      {
	// Check next buffer
	++buffer;
	interval = 0;
      }
  }
  

  bool
  NegotiationIterator::BufferTraversal::end ()
  {
    return buffer == buffers.size ();
  }


  void
  NegotiationIterator::BufferTraversal::operator++ ()
  {
    ++interval;
    findInterval ();
  }


  SpatialNegotiationData*
  NegotiationIterator::BufferTraversal::dereference ()
  {
    return &buffers[buffer][interval];
  }


  NegotiationIterator::NegotiationIterator (Implementation* impl)
    : implementation_ (impl)
  {
  }


  NegotiationIterator::NegotiationIterator (NegotiationIntervals& buffer)
    : implementation_ (new IntervalTraversal (buffer))
  {
  }
  

  NegotiationIterator::NegotiationIterator
  (std::vector<NegotiationIntervals>& buffers)
    : implementation_ (new BufferTraversal (buffers))
  {
  }
  

  SpatialNegotiator::SpatialNegotiator (IndexMap* ind, Index::Type type_)
    : indices (ind->copy ()), type (type_)
  {
  }


  SpatialNegotiator::~SpatialNegotiator ()
  {
    delete indices;
  }


  void
  SpatialNegotiator::negotiateWidth ()
  {
    // First determine local least upper bound and width
    int u = -1;
    int w = 0;
    for (IndexMap::iterator i = indices->begin ();
	 i != indices->end ();
	 ++i)
      {
	if (i->end () > u)
	  u = i->end ();
	w += i->end () - i->begin ();
      }
    // Now take maximum over all processes
    std::vector<int> m (nProcesses);
    comm.Allgather (&u, 1, MPI::INT, &m[0], 1, MPI::INT);
    for (int i = 0; i < nProcesses; ++i)
      if (m[i] > u)
	u = m[i];
    width = u;
    comm.Allgather (&w, 1, MPI::INT, &m[0], 1, MPI::INT);
    for (int i = 0; i < nProcesses; ++i)
      if (m[i] > w)
	w = m[i];
    maxLocalWidth_ = w;
  }

  
  void
  SpatialOutputNegotiator::negotiateWidth (MPI::Intercomm intercomm)
  {
    SpatialNegotiator::negotiateWidth ();
    if (localRank == 0)
      {
	int remoteWidth;
	intercomm.Recv (&remoteWidth, 1, MPI::INT, 0, 0); //*fixme* tag
	if (remoteWidth != width)
	  {
	    std::ostringstream msg;
	    msg << "sender and receiver width mismatch ("
		<< width << " != " << remoteWidth << ")";
	    error (msg.str ());
	  }
      }
  }

  
  void
  SpatialInputNegotiator::negotiateWidth (MPI::Intercomm intercomm)
  {
    SpatialNegotiator::negotiateWidth ();
    if (localRank == 0)
      intercomm.Send (&width, 1, MPI::INT, 0, 0); //*fixme* tag
  }

  
  NegotiationIterator
  SpatialNegotiator::canonicalDistribution (int width, int nProcesses)
  {
    class Wrapper : public NegotiationIterator::Implementation {
      int i;
      int r;
      int w;
      int nPerProcess;
      SpatialNegotiationData data;
    public:
      Wrapper (int width, int nProcesses)
	: i (0), r (0), w (width)
      {
	nPerProcess = w / nProcesses;
	if (w % nProcesses > 0)
	  ++nPerProcess;
      }
      bool end () { return i >= w; }
      void operator++ () { ++r; i += nPerProcess; }
      SpatialNegotiationData* dereference ()
      {
	int high = std::min (i + nPerProcess, w);
	data = SpatialNegotiationData (IndexInterval (i, high, 0), r);
	return &data;
      }
      Implementation* copy ()
      {
	return new Wrapper (*this);
      }
    };

    return NegotiationIterator (new Wrapper (width, nProcesses));
  }

  
  NegotiationIterator
  SpatialNegotiator::wrapIntervals (IndexMap::iterator beg,
				    IndexMap::iterator end,
				    Index::Type type,
				    int rank)
  {
    class Wrapper : public NegotiationIterator::Implementation {
      IndexMap::iterator end_;
    protected:
      SpatialNegotiationData data;
      IndexMap::iterator i;
      int rank_;
    public:
      Wrapper (IndexMap::iterator beg,
	       IndexMap::iterator end,
	       int rank)
	: i (beg), end_ (end), rank_ (rank)
      {
      }
      bool end () { return i == end_; }
      void operator++ () { ++i; }
    };

    class GlobalWrapper : public Wrapper {
    public:
      GlobalWrapper (IndexMap::iterator beg,
		     IndexMap::iterator end,
		     int rank)
	: Wrapper (beg, end, rank)
      {
      }
      SpatialNegotiationData* dereference ()
      {
	data = SpatialNegotiationData (*i, rank_);
	data.setLocal (0);
	return &data;
      }
      Implementation* copy ()
      {
	return new GlobalWrapper (*this);
      }
    };
  
    class LocalWrapper : public Wrapper {
    public:
      LocalWrapper (IndexMap::iterator beg,
		    IndexMap::iterator end,
		    int rank)
	: Wrapper (beg, end, rank)
      {
      }
      SpatialNegotiationData* dereference ()
      {
	data = SpatialNegotiationData (*i, rank_);
	return &data;
      }
      Implementation* copy ()
      {
	return new LocalWrapper (*this);
      }
    };
  
    Wrapper* w;
    if (type == Index::GLOBAL)
      {
	MUSIC_LOG ("rank " << MPI::COMM_WORLD.Get_rank ()
		   << " selecting GlobalWrapper" << std::endl);
	w = new GlobalWrapper (beg, end, rank);
      }
    else
      {
	MUSIC_LOG ("rank " << MPI::COMM_WORLD.Get_rank ()
		   << " selecting LocalWrapper" << std::endl);
	w = new LocalWrapper (beg, end, rank);
      }
    return NegotiationIterator (w);
  }

  
  // Compute intersection intervals between source and dest.  Store
  // the resulting intervals with rank from source in buffer
  // belonging to rank in dest.
  void
  SpatialNegotiator::intersectToBuffers
  (std::vector<NegotiationIntervals>& source,
   std::vector<NegotiationIntervals>& dest,
   std::vector<NegotiationIntervals>& buffers)
  {
    MUSIC_LOG0 ("intersecting-vvv to " << buffers.size () << " buffers");
    // Cleanup old buffer content
    for (std::vector<NegotiationIntervals>::iterator i = buffers.begin ();
	 i != buffers.end ();
	 ++i)
      i->clear ();
    for (std::vector<NegotiationIntervals>::iterator d = dest.begin ();
	 d != dest.end ();
	 ++d)
      for (std::vector<NegotiationIntervals>::iterator s = source.begin ();
	   s != source.end ();
	   ++s)
	intersectToBuffers2 (*s, *d, buffers);
  }

  
  void
  SpatialNegotiator::intersectToBuffers
  (NegotiationIterator source,
   NegotiationIterator dest,
   std::vector<NegotiationIntervals>& buffers)
  {
    MUSIC_LOG0 ("intersecting-nnv to " << buffers.size () << " buffers");
    // Cleanup old buffer content
    for (std::vector<NegotiationIntervals>::iterator i = buffers.begin ();
	 i != buffers.end ();
	 ++i)
      i->clear ();
    intersectToBuffers2 (source, dest, buffers);
  }
    
    
  void
  SpatialNegotiator::intersectToBuffers2
  (NegotiationIterator source,
   NegotiationIterator dest,
   std::vector<NegotiationIntervals>& buffers)
  {
    if (source.end ())
      MUSIC_LOG0 ("source empty on entry");
    if (dest.end ())
      MUSIC_LOG0 ("dest empty on entry");
    while (!source.end () && !dest.end ())
      {
	MUSIC_LOG0 ("comparing " <<
		    "(" << source->begin () << ", "
		    << source->end () << ", "
		    << source->local () << ", "
		    << source->rank () << ") and ("
		    << dest->begin () << ", "
		    << dest->end () << ", "
		    << dest->local () << ", "
		    << dest->rank () << ")");
	if (source->begin () < dest->begin ())
	  if (dest->begin () < source->end ())
	    if (dest->end () < source->end ())
	      {//*fixme* put inte helper function to get overview
		SpatialNegotiationData d (dest->begin (),
					  dest->end (),
					  dest->local () - source->local (),
					  source->rank ());
		buffers[dest->rank ()].push_back (d);
		++dest;
	      }
	    else
	      {
		SpatialNegotiationData d (dest->begin (),
					  source->end (),
					  dest->local () - source->local (),
					  source->rank ());
		buffers[dest->rank ()].push_back (d);
		++source;
	      }
	  else
	    ++source;
	else
	  if (source->begin () < dest->end ())
	    if (source->end () < dest->end ())
	      {
		SpatialNegotiationData d (source->begin (),
					  source->end (),
					  dest->local () - source->local (),
					  source->rank ());
		buffers[dest->rank ()].push_back (d);
		++source;
	      }
	    else
	      {
		SpatialNegotiationData d (source->begin (),
					  dest->end (),
					  dest->local () - source->local (),
					  source->rank ());
		buffers[dest->rank ()].push_back (d);
		++dest;
	      }
	  else
	    ++dest;
      }
  }


  void
  SpatialNegotiator::send (MPI::Comm& comm,
			   int destRank,
			   NegotiationIntervals& intervals)
  {
    SpatialNegotiationData* data = &intervals[0];
    int nIntervals = intervals.size ();
    while (nIntervals >= TRANSMITTED_INTERVALS_MAX)
      {
	comm.Send (data,
		   sizeof (SpatialNegotiationData) / sizeof (int) * nIntervals,
		   MPI::INT,
		   destRank,
		   0); //*fixme* tag
	data += TRANSMITTED_INTERVALS_MAX;
	nIntervals -= TRANSMITTED_INTERVALS_MAX;
      }
    comm.Send (data,
	       sizeof (SpatialNegotiationData) / sizeof (int) * nIntervals,
	       MPI::INT,
	       destRank,
	       0); //*fixme* tag
  }


  void
  SpatialNegotiator::receive (MPI::Comm& comm,
			      int sourceRank,
			      NegotiationIntervals& intervals)
  {
    MPI::Status status;
    int nReceived;
    int nextPos = 0;
    do
      {
	intervals.resize (nextPos + TRANSMITTED_INTERVALS_MAX);
	comm.Recv (&intervals[nextPos],
		   sizeof (SpatialNegotiationData) / sizeof (int)
		   * TRANSMITTED_INTERVALS_MAX,
		   MPI::INT,
		   sourceRank,
		   0,
		   status); //*fixme* tag
	nReceived = (status.Get_count (MPI::INT)
		     / (sizeof (SpatialNegotiationData) / sizeof (int)));
	nextPos += nReceived;
      }
    while (nReceived == TRANSMITTED_INTERVALS_MAX);
    intervals.resize (nextPos);
  }


  void
  SpatialNegotiator::allToAll (std::vector<NegotiationIntervals>& out,
			       std::vector<NegotiationIntervals>& in)
  {
    if (out.size () != nProcesses || in.size () != nProcesses)
      error ("internal error in SpatialNegotiator::allToAll ()");
    in[localRank] = out[localRank];
    for (int i = 0; i < localRank; ++i)
      receive (comm, i, in[i]);
    for (int i = localRank + 1; i < nProcesses; ++i)
      send (comm, i, out[i]);
    for (int i = 0; i < localRank; ++i)
      send (comm, i, out[i]);
    for (int i = localRank + 1; i < nProcesses; ++i)
      receive (comm, i, in[i]);
  }
  
  
  NegotiationIterator
  SpatialOutputNegotiator::negotiate (MPI::Intracomm c,
				      MPI::Intercomm intercomm,
				      int remoteNProc)
  {
    MUSIC_LOG0 ("SpatialOutputNegotiator::negotiate");
    comm = c;
    MUSIC_LOG0 ("o before Get_size");
    nProcesses = comm.Get_size ();
    MUSIC_LOG0 ("o before Get_rank");
    localRank = comm.Get_rank ();
    MUSIC_LOG0 ("o after Get_rank");
    local.resize (nProcesses);
    remote.resize (remoteNProc);
    results.resize (nProcesses);

    negotiateWidth (intercomm);
    NegotiationIterator mappedDist = wrapIntervals (indices->begin (),
						    indices->end (),
						    type,
						    localRank);
    NegotiationIterator canonicalDist
      = canonicalDistribution (width, nProcesses);
    //*fixme* rename results
    intersectToBuffers (mappedDist, canonicalDist, results);

    // Send to virtual connector
    allToAll (results, local);

    // Receive from remote connector
    for (int i = 0; i < remoteNProc; ++i)
      receive (intercomm, i, remote[i]);
    
    results.resize (remoteNProc);
    intersectToBuffers (local, remote, results);

    // Send to remote connector
    for (int i = 0; i < remoteNProc; ++i)
      send (intercomm, i, results[i]);
    
    results.resize (nProcesses);
    intersectToBuffers (remote, local, results);

    // Send back to real connector
    allToAll (results, local);

    return NegotiationIterator (local);
  }
  
  
  NegotiationIterator
  SpatialInputNegotiator::negotiate (MPI::Intracomm c,
				     MPI::Intercomm intercomm,
				     int remoteNProc)
  {
    MUSIC_LOG0 ("SpatialInputNegotiator::negotiate");
    comm = c;
    MUSIC_LOG0 ("i before Get_size");
    nProcesses = comm.Get_size ();
    MUSIC_LOG0 ("i before Get_rank");
    localRank = comm.Get_rank ();
    MUSIC_LOG0 ("i after Get_rank");
    remote.resize (remoteNProc);
    
    negotiateWidth (intercomm);
    NegotiationIterator mappedDist = wrapIntervals (indices->begin (),
						    indices->end (),
						    type,
						    localRank);
    NegotiationIterator canonicalDist
      = canonicalDistribution (width, remoteNProc);

    intersectToBuffers (mappedDist, canonicalDist, remote);

    for (int i = 0; i < remoteNProc; ++i)
      send (intercomm, i, remote[i]);
    
    for (int i = 0; i < remoteNProc; ++i)
      receive (intercomm, i, remote[i]);
    
    return NegotiationIterator (remote);
  }
  
}

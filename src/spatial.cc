/*
 *  This file is part of MUSIC.
 *  Copyright (C) 2008 INCF
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

#include "music/error.hh"

#include "music/spatial.hh"

//#define MUSIC_DEBUG 1

#ifdef MUSIC_DEBUG
#define MUSIC_LOG(X) (std::cout << X << std::endl)
#define MUSIC_LOGN(N, X) { if (MPI::COMM_WORLD.Get_rank () == N) std::cout << X << std::endl; }
#define MUSIC_LOG0(X) MUSIC_LOGN (0, X)
#else
#define MUSIC_LOG(X)
#define MUSIC_LOGN(N, X)
#define MUSIC_LOG0(X)
#endif

namespace MUSIC {

  negotiation_iterator::buffer_traversal::buffer_traversal
  (std::vector<negotiation_intervals>& _buffers)
    : buffers (_buffers), buffer (0), interval (0)
  {
    find_interval ();
  }


  void
  negotiation_iterator::buffer_traversal::find_interval ()
  {
    while (buffer < buffers.size () && interval == buffers[buffer].size ())
      {
	// Check next buffer
	++buffer;
	interval = 0;
      }
  }
  

  bool
  negotiation_iterator::buffer_traversal::end ()
  {
    return buffer == buffers.size ();
  }


  void
  negotiation_iterator::buffer_traversal::operator++ ()
  {
    ++interval;
    find_interval ();
  }


  spatial_negotiation_data*
  negotiation_iterator::buffer_traversal::dereference ()
  {
    return &buffers[buffer][interval];
  }


  negotiation_iterator::negotiation_iterator (implementation* impl)
    : _implementation (impl), ref_count (new int (1))
  {
  }


  negotiation_iterator::negotiation_iterator
  (std::vector<negotiation_intervals>& buffers)
    : _implementation (new buffer_traversal (buffers)),
      ref_count (new int (1))
  {
  }
  

  spatial_negotiator::spatial_negotiator (index_map* ind, index::type _type)
    : indices (ind->copy ()), type (_type)
  {
  }


  void
  spatial_negotiator::negotiate_width ()
  {
    //*fixme* insert error checking for width
    // First determine local least upper bound
    int w = -1;
    for (index_map::iterator i = indices->begin ();
	 i != indices->end ();
	 ++i)
      if (i->end () > w)
	w = i->end ();
    // Now take maximum over all processes
    std::vector<int> m (n_processes);
    comm.Allgather (&w, 1, MPI::INTEGER, &m[0], 1, MPI::INTEGER);
    for (int i = 0; i < n_processes; ++i)
      if (m[i] > w)
	w = m[i];
    width = w;
  }

  
  negotiation_iterator
  spatial_negotiator::canonical_distribution (int width, int n_processes)
  {
    class wrapper : public negotiation_iterator::implementation {
      int i;
      int r;
      int w;
      int n_per_process;
      spatial_negotiation_data data;
    public:
      wrapper (int width, int n_processes)
	: i (0), r (0), w (width)
      {
	n_per_process = w / n_processes;
	if (w % n_processes > 0)
	  ++n_per_process;
      }
      bool end () { return i >= w; }
      void operator++ () { ++r; i += n_per_process; }
      spatial_negotiation_data* dereference ()
      {
	int high = std::min (i + n_per_process, w);
	data = spatial_negotiation_data (index_interval (i, high, 0), r);
	return &data;
      }
    };

    return negotiation_iterator (new wrapper (width, n_processes));
  }

  
  negotiation_iterator
  spatial_negotiator::wrap_intervals (index_map::iterator beg,
				      index_map::iterator end,
				      int rank)
  {
    class wrapper : public negotiation_iterator::implementation {
      index_map::iterator i;
      index_map::iterator _end;
      int _rank;
      spatial_negotiation_data data;
    public:
      wrapper (index_map::iterator beg,
	       index_map::iterator end,
	       int rank)
	: i (beg), _end (end), _rank (rank)
      {
      }
      bool end () { return i == _end; }
      void operator++ () { ++i; }
      spatial_negotiation_data* dereference ()
      {
	data = spatial_negotiation_data (*i, _rank);
	return &data;
      }
    };

    return negotiation_iterator (new wrapper (beg, end, rank));
  }

  
  void
  spatial_negotiator::log (spatial_negotiation_data& d, int rank)
  {
    MUSIC_LOG0 ("(" << d.begin () << ", "
		<< d.end () << ", "
		<< d.local () << ", "
		<< d.rank () << ") -> " << rank);
  }

  
  void
  spatial_negotiator::log (spatial_negotiation_data& d)
  {
    MUSIC_LOG0 ("(" << d.begin () << ", "
		<< d.end () << ", "
		<< d.local () << ", "
		<< d.rank () << ")");
  }

  
  void
  spatial_negotiator::log (int n, spatial_negotiation_data& d)
  {
    MUSIC_LOGN (n,
		n << ": (" << d.begin () << ", "
		<< d.end () << ", "
		<< d.local () << ", "
		<< d.rank () << ")");
  }

  
  // Compute intersection intervals between source and dest.  Store
  // the resulting intervals with rank from source in buffer
  // belonging to rank in dest.
  void
  spatial_negotiator::intersect_to_buffers
  (negotiation_iterator source,
   negotiation_iterator dest,
   std::vector<negotiation_intervals>& buffers)
  {
    MUSIC_LOG0 ("intersecting to " << buffers.size () << " buffers");
    // Cleanup old buffer content
    for (int i = 0; i < buffers.size (); ++i)
      buffers[i].clear ();
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
		spatial_negotiation_data d (dest->begin (),
					    dest->end (),
					    source->local () - dest->local (),
					    source->rank ());
		log (d, dest->rank ());
		buffers[dest->rank ()].push_back (d);
		++dest;
	      }
	    else
	      {
		spatial_negotiation_data d (dest->begin (),
					    source->end (),
					    source->local () - dest->local (),
					    source->rank ());
		log (d, dest->rank ());
		buffers[dest->rank ()].push_back (d);
		++source;
	      }
	  else
	    ++source;
	else
	  if (source->begin () < dest->end ())
	    if (source->end () < dest->end ())
	      {
		spatial_negotiation_data d (source->begin (),
					    source->end (),
					    source->local () - dest->local (),
					    source->rank ());
		log (d, dest->rank ());
		buffers[dest->rank ()].push_back (d);
		++source;
	      }
	    else
	      {
		spatial_negotiation_data d (source->begin (),
					    dest->end (),
					    source->local () - dest->local (),
					    source->rank ());
		log (d, dest->rank ());
		buffers[dest->rank ()].push_back (d);
		++dest;
	      }
	  else
	    ++dest;
      }
  }


  void
  spatial_negotiator::send (MPI::Comm& comm,
			    int dest_rank,
			    negotiation_intervals& intervals)
  {
    comm.Send (&intervals[0], 4 * intervals.size (), MPI::INTEGER, dest_rank, 0); //*fixme* size, tag
  }


  void
  spatial_negotiator::receive (MPI::Comm& comm,
			       int source_rank,
			       negotiation_intervals& intervals)
  {
    intervals.resize (1000);//*fixme* re-sending protocol
    MPI::Status status;
    comm.Recv (&intervals[0], 4 * intervals.size (), MPI::INTEGER, source_rank, 0, status); //*fixme* size, tag
    intervals.resize (status.Get_count (MPI::INTEGER) / 4);
  }


  void
  spatial_negotiator::all_to_all (std::vector<negotiation_intervals>& out,
				  std::vector<negotiation_intervals>& in)
  {
    if (out.size () != n_processes || in.size () != n_processes)
      error ("internal error in spatial_negotiator::all_to_all ()");
    in[local_rank] = out[local_rank];
    for (int i = 0; i < local_rank; ++i)
      receive (comm, i, in[i]);
    for (int i = local_rank + 1; i < n_processes; ++i)
      send (comm, i, out[i]);
    for (int i = 0; i < local_rank; ++i)
      send (comm, i, out[i]);
    for (int i = local_rank + 1; i < n_processes; ++i)
      receive (comm, i, in[i]);
  }
  
  
  negotiation_iterator
  spatial_output_negotiator::negotiate (MPI::Intracomm c,
					MPI::Intercomm intercomm,
					int remote_n_proc)
  {
    comm = c;
    n_processes = comm.Get_size ();
    local_rank = comm.Get_rank ();
    local.resize (n_processes);
    remote.resize (remote_n_proc);
    results.resize (n_processes);

    negotiate_width ();
    negotiation_iterator mapped_dist = wrap_intervals (indices->begin (),
						       indices->end (),
						       local_rank);
    negotiation_iterator canonical_dist
      = canonical_distribution (width, n_processes);
    //*fixme* rename results
    MUSIC_LOG0 ("intersect_to_buffers (mapped_dist, canonical_dist, results)");
    intersect_to_buffers (mapped_dist, canonical_dist, results);

    // Send to virtual connector
    all_to_all (results, local);

    // Receive from remote connector
    for (int i = 0; i < remote_n_proc; ++i)
      receive (intercomm, i, remote[i]);
    
    results.resize (remote_n_proc);
    MUSIC_LOG0 ("local:");
    for (int i = 0; i < local.size (); ++i)
      {
	MUSIC_LOG0 ("buffer " << i << ":");
	for (int j = 0; j < local[i].size (); ++j)
	  log (local[i][j]);
      }
    MUSIC_LOG0 ("remote:");
    for (int i = 0; i < remote.size (); ++i)
      {
	MUSIC_LOG0 ("buffer " << i << ":");
	for (int j = 0; j < remote[i].size (); ++j)
	  log (remote[i][j]);
      }
    MUSIC_LOG0 ("intersect_to_buffers (local, remote, results)");
    intersect_to_buffers (local, remote, results);

    // Send to remote connector
    for (int i = 0; i < remote_n_proc; ++i)
      send (intercomm, i, results[i]);
    
    results.resize (n_processes);
    MUSIC_LOG0 ("intersect_to_buffers (remote, local, results)");
    intersect_to_buffers (remote, local, results);

    // Send back to real connector
    all_to_all (results, local);

    return negotiation_iterator (local);
  }
  
  
  negotiation_iterator
  spatial_input_negotiator::negotiate (MPI::Intracomm c,
				       MPI::Intercomm intercomm,
				       int remote_n_proc)
  {
    comm = c;
    n_processes = comm.Get_size ();
    local_rank = comm.Get_rank ();
    remote.resize (remote_n_proc);
    
    negotiate_width ();
    negotiation_iterator mapped_dist = wrap_intervals (indices->begin (),
						       indices->end (),
						       local_rank);
    negotiation_iterator canonical_dist
      = canonical_distribution (width, remote_n_proc);
    MUSIC_LOGN (2, "2: intersect_to_buffers (local, remote, results)");
    intersect_to_buffers (mapped_dist, canonical_dist, remote);

    for (int i = 0; i < remote_n_proc; ++i)
      send (intercomm, i, remote[i]);
    
    for (int i = 0; i < remote_n_proc; ++i)
      receive (intercomm, i, remote[i]);
    
    MUSIC_LOGN (2, "2: remote:");
    for (int i = 0; i < remote.size (); ++i)
      {
	MUSIC_LOGN (2, "2: buffer " << i << ":");
	for (int j = 0; j < remote[i].size (); ++j)
	  log (2, remote[i][j]);
      }
    
    return negotiation_iterator (remote);
  }
  
}

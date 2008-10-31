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

#ifndef MUSIC_NEGOTIATOR_HH

#include <mpi.h>
#include <vector>
#include <memory>

#include <music/index_map.hh>

namespace MUSIC {

  const int TRANSMITTED_INTERVALS_MAX = 10000;

  class spatial_negotiation_data {
    index_interval _interval;
    int _rank;
  public:
    spatial_negotiation_data () { }
    spatial_negotiation_data (index_interval i, int r)
      : _interval (i), _rank (r) { }
    spatial_negotiation_data (int b, int e, int l, int r)
      : _interval (b, e, l), _rank (r) { }
    const index_interval& interval () const { return _interval; }
    int begin () const { return _interval.begin (); }
    int end () const { return _interval.end (); }
    int local () const { return _interval.local (); }
    int rank () const { return _rank; }
  };


  typedef std::vector<spatial_negotiation_data> negotiation_intervals;
  

  class negotiation_iterator {
  public:
    class implementation {
    public:
      virtual bool end () = 0;
      virtual void operator++ () = 0;
      virtual spatial_negotiation_data* dereference () = 0;
      virtual implementation* copy () = 0;
    };

    class interval_traversal : public implementation {
      negotiation_intervals& buffer;
      int interval;
    public:
      interval_traversal (negotiation_intervals& _buffer)
	: buffer (_buffer), interval (0) { }
      bool end () { return interval == buffer.size (); }
      void operator++ () { ++interval; }
      spatial_negotiation_data* dereference () { return &buffer[interval]; }
      virtual implementation* copy () { return new interval_traversal (*this); }
    };

    class buffer_traversal : public implementation {
      std::vector<negotiation_intervals>& buffers;
      int buffer;
      int interval;
      void find_interval ();
    public:
      buffer_traversal (std::vector<negotiation_intervals>& buffers);
      bool end ();
      void operator++ ();
      spatial_negotiation_data* dereference ();
      virtual implementation* copy () { return new buffer_traversal (*this); }
    };

  private:
    implementation* _implementation;
  public:
    negotiation_iterator (implementation* impl);
    negotiation_iterator (negotiation_intervals& buffer);
    negotiation_iterator (std::vector<negotiation_intervals>& buffers);
    ~negotiation_iterator ()
    {
      delete _implementation;
    }
    negotiation_iterator (const negotiation_iterator& i)
      : _implementation (i._implementation->copy ())
    {
    }
    const negotiation_iterator& operator= (const negotiation_iterator& i)
    {
      delete _implementation;
      _implementation = i._implementation->copy ();
      return *this;
    }
    bool end () { return _implementation->end (); }
    negotiation_iterator& operator++ ()
    {
      ++*_implementation;
      return *this;
    };
    spatial_negotiation_data* operator-> ()
    {
      return _implementation->dereference ();
    }
  };
  

  // The spatial_negotiator negotiates with the remote application how
  // to redistribute data over a port pair.  This is done using the
  // algorithm described in Djurfeldt and Ekeberg (2009) with memory
  // complexity O (N / P) where N is the port width and P is the
  // number of MPI processes.

  class spatial_negotiator {
  protected:
    MPI::Intracomm comm;
    index_map* indices;
    index::type type;
    std::vector<negotiation_intervals> remote;
    int width;
    int local_rank;
    int n_processes;
  public:
    spatial_negotiator (index_map* indices, index::type type);
    void log (spatial_negotiation_data& d, int rank);
    void log (spatial_negotiation_data& d);
    void log (int n, spatial_negotiation_data& d);
    void negotiate_width ();
    negotiation_iterator wrap_intervals (index_map::iterator beg,
					 index_map::iterator end,
					 int rank);
    void send (MPI::Comm& comm, int dest_rank,
	       negotiation_intervals& intervals);
    void receive (MPI::Comm& comm, int source_rank,
		  negotiation_intervals& intervals);
    void all_to_all (std::vector<negotiation_intervals>& out,
		     std::vector<negotiation_intervals>& in);
    negotiation_iterator canonical_distribution (int width, int n_processes);
    void intersect_to_buffers (std::vector<negotiation_intervals>& source,
			       negotiation_iterator dest,
			       std::vector<negotiation_intervals>& buffers);
    void intersect_to_buffers (negotiation_iterator source,
			       negotiation_iterator dest,
			       std::vector<negotiation_intervals>& buffers);
  private:
    void intersect_to_buffers_2 (negotiation_iterator source,
				 negotiation_iterator dest,
				 std::vector<negotiation_intervals>& buffers);
  public:
    virtual negotiation_iterator negotiate (MPI::Intracomm comm,
					    MPI::Intercomm intercomm,
					    int remote_n_proc) = 0;
  };


  class spatial_output_negotiator : public spatial_negotiator {
    std::vector<negotiation_intervals> local;
    std::vector<negotiation_intervals> results;
  public:
    spatial_output_negotiator (index_map* indices, index::type type)
      : spatial_negotiator (indices, type) { }
    negotiation_iterator negotiate (MPI::Intracomm comm,
				    MPI::Intercomm intercomm,
				    int remote_n_proc);
  };

  
  class spatial_input_negotiator : public spatial_negotiator {
  public:
    spatial_input_negotiator (index_map* indices, index::type type)
      : spatial_negotiator (indices, type) { }
    negotiation_iterator negotiate (MPI::Intracomm comm,
				    MPI::Intercomm intercomm,
				    int remote_n_proc);
  };

}

#define MUSIC_NEGOTIATOR_HH
#endif

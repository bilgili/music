/*
 *  This file is part of MUSIC.
 *  Copyright (C) 2007, 2008 INCF
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

#ifndef MUSIC_INDEX_MAP_HH

#include <memory>

#include <music/interval.hh>

namespace MUSIC {

  class index {
  public:
    enum type { GLOBAL, LOCAL };
  };

  class global_index : public index {
    int id;
  public:
    global_index () { }
    global_index (int i) : id (i) { }
    operator int () const { return id; }
  };

  class local_index : public index {
    int id;
  public:
    local_index () { }
    local_index (int i) : id (i) { }
    operator int () const { return id; }
  };

  class index_interval : public interval {
    local_index _local;
  public:
    index_interval () { }
    index_interval (global_index b, global_index e, local_index l)
      : interval (b, e), _local (l) { }
    local_index local () const { return _local; }
  };

  class index_map {
  public:
    class iterator_implementation {
    public:
      virtual const index_interval operator* () = 0;
      virtual const index_interval* dereference () = 0;
      virtual bool is_equal (iterator_implementation* i) const = 0;
      virtual void operator++ () = 0;      
    };

    
    class iterator {
      iterator_implementation* _implementation;
      int* ref_count; //*fixme* use some other mechanism, e.g. template
    public:
      iterator (iterator_implementation* impl)
	: _implementation (impl), ref_count (new int (1)) { }
      ~iterator ()
      {
	if (--*ref_count == 0)
	  {
	    delete _implementation;
	    delete ref_count;
	  }
      }
      iterator (const iterator& i)
	: _implementation (i._implementation), ref_count (i.ref_count)
      {
	++*ref_count;
      }
      const iterator& operator= (const iterator& i)
      {
	if (--*ref_count == 0)
	  {
	    delete _implementation;
	    delete ref_count;
	  }
	_implementation = i._implementation;
	ref_count = i.ref_count;
	++*ref_count;
	return *this;
      }
      iterator_implementation* implementation () const
      {
	return _implementation;
      }
      const index_interval operator* ();
      const index_interval* operator-> ();
      bool operator== (const iterator& i) const;
      bool operator!= (const iterator& i) const;
      iterator& operator++ ();
    };
    
    
    virtual iterator begin () = 0;
    virtual const iterator end () const = 0;

    virtual index_map* copy () = 0;
  };

}

#define MUSIC_INDEX_MAP_HH
#endif

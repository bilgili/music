/*
 *  This file is distributed together with MUSIC.
 *  Copyright (C) 2008, 2009 Mikael Djurfeldt
 *
 *  This interval table implementation is free software; you can
 *  redistribute it and/or modify it under the terms of the GNU
 *  General Public License as published by the Free Software
 *  Foundation; either version 3 of the License, or (at your option)
 *  any later version.
 *
 *  The interval table implementation is distributed in the hope that
 *  it will be useful, but WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MUSIC_INTERVAL_TABLE_HH

#include <vector>
#include <map>
#include <limits>
#include <algorithm>
//#include <hash_map>

extern "C" {
#include <assert.h>
}

#include "ordered_ilist.hh"

namespace MUSIC {

  template<class PointType, class IntervalType, class DataType>
  class IntervalTable {
  private:
    typedef unsigned short DataIndex;
    static const unsigned int MAXDATA = 65536;

    typedef OrderedIList<DataIndex> IList;

    class PreEntry {
      IList list_;
      IList hint_;
    public:
      PreEntry () : list_ (IList::NIL), hint_ (list_) { }
      IList list () { return list_; }
      void insert (DataIndex i) { hint_ = list_.insert (i, hint_); }
    };

    // was hash_map
    typedef std::map<DataIndex, PreEntry> PreEntryMap;
    PreEntryMap* preEntryMap_;

    class Entry {
      PointType key_;
      IList ls_;
    public:
      Entry (PointType key, IList ls) : key_ (key), ls_ (ls) { }
      PointType key () { return key_; }
      IList list () { return ls_; }
    };

    std::vector<Entry> entryTable_;
    std::vector<DataType> data_;

    class SortCriterion {
      static const unsigned int dataSize = sizeof (DataType);
      struct Ptrs {
	void* p0;
	void* p1;
      };
    public:
      bool operator() (const DataType& x, const DataType& y) const
      {
	if (sizeof (DataType) != sizeof (Ptrs))
	  assert (sizeof (DataType) == sizeof (Ptrs));
	const DataType* px = &x;
	const Ptrs u = *reinterpret_cast<const Ptrs*> (px);
	const DataType* py = &y;
	const Ptrs v = *reinterpret_cast<const Ptrs*> (py);
	return u.p1 < v.p1 || (u.p1 == v.p1 && u.p0 < v.p0);
      }
    };

    typedef std::map<DataType, DataIndex, SortCriterion> DataMap;
    DataMap* dataMap_;

    PointType lowerBound_;
    PointType rangeSize_;
    unsigned int tableSize_;
    void recordProperties (const IntervalType& ival);

    unsigned int nIntervals_;

  public:
    class Action {
    public:
      virtual void operator() (DataType& data) = 0;
    };

    IntervalTable ()
      : lowerBound_ (std::numeric_limits<PointType>::max ()),
	rangeSize_ (std::numeric_limits<PointType>::min ()),
	tableSize_ (0),
	nIntervals_ (0)
    {
      preEntryMap_ = new PreEntryMap ();
      dataMap_ = new DataMap ();
    }
    void add (const IntervalType& i, const DataType& data);
    void build ();
    void search (PointType point, Action* a);
    int size () const { return nIntervals_; }
    int tableSize () const { return entryTable_.size (); }
  };

  template<class PointType, class IntervalType, class DataType>
  void
  IntervalTable<PointType, IntervalType, DataType>::recordProperties
  (const IntervalType& ival)
  {
    lowerBound_ = std::min (lowerBound_, ival.begin ());
    rangeSize_ = std::max (rangeSize_, ival.end ());
    nIntervals_ += 1;
  }

  template<class PointType, class IntervalType, class DataType>
  void
  IntervalTable<PointType, IntervalType, DataType>::add (const IntervalType& ival,
							 const DataType& data)
  {
    // assign a DataIndex to data
    typename DataMap::iterator pos = dataMap_->find (data);
    DataIndex dataIndex;
    if (pos == dataMap_->end ())
      {
	dataIndex = data_.size ();
	assert (dataIndex < MAXDATA);
	data_.push_back (data);
	(*dataMap_)[data] = dataIndex;
      }
    else
      dataIndex = pos->second;

    recordProperties (ival);

    for (PointType k = ival.begin (); k < ival.end (); ++k)
      (*preEntryMap_)[k].insert (dataIndex);
  }


  template<class PointType, class IntervalType, class DataType>
  void
  IntervalTable<PointType, IntervalType, DataType>::build ()
  {
    delete dataMap_;
    // trim data_;
    std::vector<DataType> (data_).swap (data_);
    tableSize_ = preEntryMap_->size ();
    entryTable_.reserve (tableSize_ + 2);
    // backward sentinel
    entryTable_.push_back (Entry (std::numeric_limits<PointType>::min (),
				  IList::NIL));
    for (typename PreEntryMap::iterator p = preEntryMap_->begin ();
	 p != preEntryMap_->end ();
	 ++p)
      entryTable_.push_back (Entry (p->first, p->second.list ()));
    // forward sentinel
    entryTable_.push_back (Entry (std::numeric_limits<PointType>::max (),
				  IList::NIL));
    delete preEntryMap_;
    rangeSize_ = rangeSize_ - lowerBound_ + 1;
  }
  
  
  template<class PointType, class IntervalType, class DataType>
  void
  IntervalTable<PointType, IntervalType, DataType>::search (PointType p, Action* a)
  {
    // obtain approximate position
    PointType i = ((p - lowerBound_) * tableSize_) / rangeSize_ + 1;

    if (i < 0 || i >= entryTable_.size ())
      return;

    if (p != entryTable_[i].key ())
      {
	// search forward
	if (p > entryTable_[i].key ())
	  {
	    do
	      ++i;
	    while (p > entryTable_[i].key ());
	    if (p != entryTable_[i].key ())
	      return;
	  }
	else
	  // search backward
	  {
	    do
	      --i;
	    while (p < entryTable_[i].key ());
	    if (p != entryTable_[i].key ())
	      return;
	  }
      }
    
    IList ls = entryTable_[i].list ();
    for (IList::iterator k = ls.begin (); k != ls.end (); ++k)
      (*a) (data_[*k]);
  }

}

#define MUSIC_INTERVAL_TABLE_HH
#endif

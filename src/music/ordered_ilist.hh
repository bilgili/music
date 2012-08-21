/*
 *  This file is distributed together with MUSIC.
 *  Copyright (C) 2012 Mikael Djurfeldt
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

#ifndef MUSIC_ORDERED_ILIST_HH

#include <vector>

extern "C" {
#include <assert.h>
}

namespace MUSIC {

  template<class DataType>
  class OrderedIList {
    typedef int ListPtr;

    static const ListPtr NILPTR = -1;

    struct Node {
      Node (DataType b, DataType e, ListPtr n)
	: begin_ (b), end_ (e), next_ (n)
      { }
      DataType begin_;
      DataType end_; // inclusive
      ListPtr next_;
    };

    static std::vector<Node> node_;
    static OrderedIList freePtr_;

    ListPtr lptr_;

    DataType begin_ () const { return node_[lptr_].begin_; }
    void setBegin_ (const DataType x) { node_[lptr_].begin_ = x; }
    DataType end_ () const { return node_[lptr_].end_; }
    void setEnd_ (const DataType x) { node_[lptr_].end_ = x; }
    OrderedIList next_ () const { return OrderedIList (node_[lptr_].next_); }
    void setNext_ (const OrderedIList list) const { node_[lptr_].next_ = list.lptr_; }
    OrderedIList cons (const DataType b, const DataType e, const OrderedIList n);
    void free (const OrderedIList list);
  public:
    static const OrderedIList NIL;

    class iterator {
      OrderedIList list_;
      DataType i_;
    public:
      iterator (OrderedIList list)
      : list_ (list), i_ (list.isEmpty () ? 0 : list.begin_ ())
      { }
      bool operator!= (const iterator& i) const
      {
	return list_ != i.list_ || i_ != i.i_;
      }
      iterator& operator++ ();
      const DataType operator* () { return i_; }
    };

    OrderedIList () : lptr_ (NILPTR) { }
    OrderedIList (ListPtr ptr) : lptr_ (ptr) { }
    bool operator== (const OrderedIList& list) const { return lptr_ == list.lptr_; }
    bool operator!= (const OrderedIList& list) const { return lptr_ != list.lptr_; }
    bool isEmpty () const { return lptr_ == NILPTR; }
    OrderedIList insert (DataType i) { return insert (i, *this); }
    OrderedIList insert (DataType i, OrderedIList hint);
    iterator begin () const { return iterator (*this); }
    iterator end () const { return iterator (NIL); }
  };

  template<class DataType>
  std::vector<typename OrderedIList<DataType>::Node> OrderedIList<DataType>::node_;

  template<class DataType>
  OrderedIList<DataType> OrderedIList<DataType>::freePtr_
  = OrderedIList<DataType>::NIL;

  template<class DataType>
  const OrderedIList<DataType> OrderedIList<DataType>::NIL;

  template<class DataType>
  OrderedIList<DataType>
  OrderedIList<DataType>::cons (const DataType b,
				const DataType e,
				const OrderedIList n)
  {
    if (freePtr_.isEmpty ())
      {
	// create a new node
	ListPtr lptr = node_.size ();
	node_.push_back (Node (b, e, n.lptr_));
	return OrderedIList (lptr);
      }
    else
      {
	// unlink from freelist
	OrderedIList list = freePtr_;
	freePtr_ = freePtr_.next_ ();
	list.setBegin_ (b);
	list.setEnd_ (e);
	list.setNext_ (n);
	return list;
      }
  }

  template<class DataType>
  void
  OrderedIList<DataType>::free (const OrderedIList list)
  {
    list.setNext_ (freePtr_);
    freePtr_ = list;
  }

  template<class DataType>
  typename OrderedIList<DataType>::iterator&
  OrderedIList<DataType>::iterator::operator++ ()
  {
    ++i_;
    if (i_ > list_.end_ ())
      {
	list_ = list_.next_ ();
	i_ = list_.isEmpty () ? 0 : list_.begin_ ();
      }
    return *this;
  }

  template<class DataType>
  OrderedIList<DataType>
  OrderedIList<DataType>::insert (DataType i, OrderedIList hint)
  {
    if (hint.isEmpty ())
      {
	if (isEmpty ())
	  return *this = cons (i, i, NIL);
	if (i < begin_ ())
	  {
	    if (i + 1 == begin_ ())
	      setBegin_ (i);
	    else
	      *this = cons (i, i, *this);
	    return *this;
	  }
	OrderedIList list = *this;
	while (true)
	  {
	    OrderedIList next = list.next_ ();
	    assert (i > list.end_ ());
	    if (next.isEmpty ())
	      {
		if (i == list.end_ () + 1)
		  {
		    list.setEnd_ (i);
		    return list;
		  }
	      }
	    else
	      {
		if (i == list.end_ () + 1)
		  {
		    if (i + 1 == next.begin_ ())
		      {
			list.setEnd_ (next.end_ ());
			list.setNext_ (next.next_ ());
			free (next);
		      }
		    else
		      {
			list.setEnd_ (i);
		      }
		    return list;
		  }
	      }
	    if (next.isEmpty ())
	      {
		next = cons (i, i, NIL);
		list.setNext_ (next);
		return next;
	      }
	    if (i < next.begin_ ())
	      {
		if (i + 1 == next.begin_ ())
		  next.setBegin_ (i);
		else
		  {
		    next = cons (i, i, next);
		    list.setNext_ (next);
		  }
		return next;
	      }
	    list = next;
	  }
      }
    else
      {
	if (i < hint.begin_ () || !hint.next_ ().isEmpty ())
	  return insert (i, NIL);
	assert (i > end_ ());
	if (i == hint.end_ () + 1)
	  {
	    hint.setEnd_ (i);
	    return hint;
	  }
	else
	  {
	    OrderedIList next = cons (i, i, NIL);
	    hint.setNext_ (next);
	    return next;
	  }
      }
  }

}

#define MUSIC_ORDERED_ILIST_HH
#endif // MUSIC_ORDERED_ILIST_HH

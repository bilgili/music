/*
 *  This file is distributed together with MUSIC.
 *  Copyright (C) 2008, 2009 Mikael Djurfeldt
 *
 *  This interval tree implementation is free software; you can
 *  redistribute it and/or modify it under the terms of the GNU
 *  General Public License as published by the Free Software
 *  Foundation; either version 3 of the License, or (at your option)
 *  any later version.
 *
 *  The interval tree implementation is distributed in the hope that
 *  it will be useful, but WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MUSIC_INTERVAL_TREE_HH

#include <vector>
#include <limits>
#include <algorithm>

namespace MUSIC {

  template<class PointType, class DataType>
  class IntervalTree {
    static const int ROOT = 0;

    class NodeType {
      PointType maxEnd_;
      DataType data_;
    public:
      NodeType () : maxEnd_ (noNode ()) { }
      NodeType (const DataType& d) : maxEnd_ (noNode ()), data_ (d) { }
      NodeType (const PointType m, const DataType& d)
	: maxEnd_ (m), data_ (d) { }
      static PointType noNode () {
	return std::numeric_limits<PointType>::min ();
      }
      bool operator< (const NodeType& other) const {
	return begin () < other.begin ();
      }
      DataType& data () { return data_; }
      PointType begin () const { return data_.begin (); }
      PointType end () const { return data_.end (); }
      PointType maxEnd () const { return maxEnd_; }
    };

    std::vector<NodeType> nodes;
    int leftChild (int i) const { return 2 * i + 1; }
    int rightChild (int i) const { return 2 * i + 2; }
    int computeSize () const;
    PointType build (std::vector<NodeType>& dest, int l, int r, int i);
  public:
    class Action {
    public:
      virtual void operator() (DataType& data) = 0;
    };
  private:
    void search (int i, PointType point, Action* a);
  public:
    void add (const DataType& data);
    void build ();
    void search (PointType point, Action* a);
  };

  template<class PointType, class DataType>
  void
  IntervalTree<PointType, DataType>::add (const DataType& data)
  {
    nodes.push_back (NodeType (data));
  }


  template<class PointType, class DataType>
  int
  IntervalTree<PointType, DataType>::computeSize () const
  {
    int b = 2;
    int s = nodes.size () + 1;
    while (s != 0)
      {
	s >>= 1;
	b <<= 1;
      }
    return b;
  }

  
  template<class PointType, class DataType>
  void
  IntervalTree<PointType, DataType>::build ()
  {
    sort (nodes.begin (), nodes.end ());
    std::vector<NodeType> newNodes (computeSize ());
    build (newNodes, 0, nodes.size (), ROOT);
    nodes = newNodes;
  }
  
  
  template<class PointType, class DataType>
  PointType
  IntervalTree<PointType, DataType>::build (std::vector<NodeType>& dest,
					    int l,
					    int r,
					    int i)
  {
    if (l < r) // sequence not empty
      {
	int m = (l + r) / 2;
	PointType max = nodes[m].end ();
	PointType end = build (dest, l, m, leftChild (i));
	max = end < max ? max : end;
	end = build (dest, m + 1, r, rightChild (i));
	max = end < max ? max : end;
	dest[i] = NodeType (max, nodes[m].data ());
	return max;
      }
    else
      return NodeType::noNode ();
  }


  template<class PointType, class DataType>
  void
  IntervalTree<PointType, DataType>::search (PointType p, Action* a)
  {
    search (ROOT, p, a);
  }

  template<class PointType, class DataType>
  void
  IntervalTree<PointType, DataType>::search (int i, PointType p, Action* a)
  {
    // this condition both checks if the point is to the right of all
    // nodes in this subtree and stops recursion at NO_NODE values
    if (p >= nodes[i].maxEnd ())
      return;

    search (leftChild (i), p, a);

    if (p < nodes[i].data ().begin ())
      // point is to the left of this interval and of the right subtree
      return;

    if (p < nodes[i].data ().end ())
      // perform action on this interval
      (*a) (nodes[i].data ());

    search (rightChild (i), p, a);
  }
}

#define MUSIC_INTERVAL_TREE_HH
#endif

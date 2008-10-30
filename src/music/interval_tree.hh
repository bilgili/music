/*
 *  This file is distributed together with MUSIC.
 *  Copyright (C) 2008 Mikael Djurfeldt
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

  template<class point_type, class data_type>
  class interval_tree {
    static const int ROOT = 0;

    class node_type {
      point_type _max_end;
      data_type _data;
    public:
      node_type () : _max_end (no_node ()) { }
      node_type (const data_type& d) : _max_end (no_node ()), _data (d) { }
      node_type (const point_type m, const data_type& d)
	: _max_end (m), _data (d) { }
      static point_type no_node () {
	return std::numeric_limits<point_type>::min ();
      }
      bool operator< (const node_type& other) const {
	return begin () < other.begin ();
      }
      data_type& data () { return _data; }
      point_type begin () const { return _data.begin (); }
      point_type end () const { return _data.end (); }
      point_type max_end () const { return _max_end; }
    };

    std::vector<node_type> nodes;
    int left_child (int i) const { return 2 * i + 1; }
    int right_child (int i) const { return 2 * i + 2; }
    int compute_size () const;
    point_type build (std::vector<node_type>& dest, int l, int r, int i);
  public:
    class action {
    public:
      virtual void operator() (data_type& data) = 0;
    };
  private:
    void search (int i, point_type point, action* a);
  public:
    void add (const data_type& data);
    void build ();
    void search (point_type point, action* a);
  };

  template<class point_type, class data_type>
  void
  interval_tree<point_type, data_type>::add (const data_type& data)
  {
    nodes.push_back (node_type (data));
  }


  template<class point_type, class data_type>
  int
  interval_tree<point_type, data_type>::compute_size () const
  {
    int b = 1;
    int s = nodes.size () + 1;
    while (s != 0)
      {
	s >>= 1;
	b <<= 1;
      }
    return b;
  }

  
  template<class point_type, class data_type>
  void
  interval_tree<point_type, data_type>::build ()
  {
    sort (nodes.begin (), nodes.end ());
    std::vector<node_type> new_nodes (compute_size ());
    build (new_nodes, 0, nodes.size (), ROOT);
    nodes = new_nodes;
  }
  
  
  template<class point_type, class data_type>
  point_type
  interval_tree<point_type, data_type>::build (std::vector<node_type>& dest,
					       int l,
					       int r,
					       int i)
  {
    if (l < r) // sequence not empty
      {
	int m = (l + r) / 2;
	point_type max = nodes[m].end ();
	point_type end = build (dest, l, m, left_child (i));
	max = end < max ? max : end;
	end = build (dest, m + 1, r, right_child (i));
	max = end < max ? max : end;
	dest[i] = node_type (max, nodes[m].data ());
	return max;
      }
    else
      return node_type::no_node ();
  }


  template<class point_type, class data_type>
  void
  interval_tree<point_type, data_type>::search (point_type p, action* a)
  {
    search (ROOT, p, a);
  }

  template<class point_type, class data_type>
  void
  interval_tree<point_type, data_type>::search (int i, point_type p, action* a)
  {
    // this condition both checks if the point is to the right of all
    // nodes in this subtree and stops recursion at NO_NODE values
    if (p >= nodes[i].max_end ())
      return;

    search (left_child (i), p, a);

    if (p < nodes[i].data ().begin ())
      // point is to the left of this interval and of the right subtree
      return;

    if (p < nodes[i].data ().end ())
      // perform action on this interval
      (*a) (nodes[i].data ());

    search (right_child (i), p, a);
  }
}

#define MUSIC_INTERVAL_TREE_HH
#endif

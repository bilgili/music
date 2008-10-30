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

#include "music/application_map.hh"
#include "music/ioutils.hh"

namespace MUSIC {

  application_map::application_map (std::istringstream& in)
  {
    read (in);
  }

  
  application_info*
  application_map::lookup (std::string app_name)
  {
    for (iterator i = begin (); i != end (); ++i)
      {
	if (i->name () == app_name)
	  return &*i;
      }
    return 0;
  }
  

  void
  application_map::add (std::string name, int l, int n)
  {
    push_back (application_info (name, l, n));
  }

  
  void
  application_map::write (std::ostringstream& out)
  {
    out << size ();
    for (iterator i = begin (); i != end (); ++i)
      {
	out << ':';
	ioutils::write (out, i->name ());
	out << ':' << i->n_proc ();
      }
  }


  void
  application_map::read (std::istringstream& in)
  {
    int n;
    in >> n;
    int leader = 0;
    for (int i = 0; i < n; ++i)
      {
	in.ignore ();
	std::string name = ioutils::read (in);
	in.ignore ();
	int np;
	in >> np;
	push_back (application_info (name, leader, np));
	leader += np;
      }
  }

}

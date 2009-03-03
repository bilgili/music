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

#include "music/application_map.hh"
#include "music/ioutils.hh"

namespace MUSIC {

  ApplicationMap::ApplicationMap (std::istringstream& in)
  {
    read (in);
  }


  int
  ApplicationMap::nProcesses ()
  {
    int n = 0;
    for (ApplicationMap::iterator i = begin (); i != end (); ++i)
      n += i->nProc ();
    return n;
  }
  
  
  ApplicationInfo*
  ApplicationMap::lookup (std::string appName)
  {
    for (iterator i = begin (); i != end (); ++i)
      {
	if (i->name () == appName)
	  return &*i;
      }
    return 0;
  }
  

  void
  ApplicationMap::add (std::string name, int l, int n)
  {
    push_back (ApplicationInfo (name, l, n));
  }

  
  void
  ApplicationMap::write (std::ostringstream& out)
  {
    out << size ();
    for (iterator i = begin (); i != end (); ++i)
      {
	out << ':';
	IOUtils::write (out, i->name ());
	out << ':' << i->nProc ();
      }
  }


  void
  ApplicationMap::read (std::istringstream& in)
  {
    int n;
    in >> n;
    int leader = 0;
    for (int i = 0; i < n; ++i)
      {
	in.ignore ();
	std::string name = IOUtils::read (in);
	in.ignore ();
	int np;
	in >> np;
	push_back (ApplicationInfo (name, leader, np));
	leader += np;
      }
  }

}

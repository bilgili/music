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

#ifndef MUSIC_APPLICATION_MAP_HH

#include <sstream>
#include <vector>

namespace MUSIC {

  class application_info {
    std::string _name;
    int _leader;
    int _n_proc;
  public:
    application_info (std::string name, int l, int n)
      : _name (name), _leader (l), _n_proc (n) { }
    std::string name () { return _name; }
    int leader () { return _leader; }
    int n_proc () { return _n_proc; }
  };

  
  class application_map : public std::vector<application_info> {
    void read (std::istringstream& in);
  public:
    application_map () { }
    application_map (std::istringstream& in);
    application_info* lookup (std::string app_name);
    void add (std::string name, int l, int n);
    void write (std::ostringstream& out);
  };

}

#define MUSIC_APPLICATION_MAP_HH
#endif

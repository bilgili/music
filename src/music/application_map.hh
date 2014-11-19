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

#ifndef MUSIC_APPLICATION_MAP_HH

#include <sstream>
#include <vector>

namespace MUSIC {

  class ApplicationInfo {
    std::string name_;
    int leader_;
    int nProc_;
  public:
    ApplicationInfo (std::string appName, int l, int n)
      : name_ (appName), leader_ (l), nProc_ (n) { }
    std::string name () { return name_; }
    int leader () { return leader_; }
    int nProc () { return nProc_; }
  };

  
  class ApplicationMap : public std::vector<ApplicationInfo> {
    void read (std::istringstream& in);
  public:
    ApplicationMap () { }
    ApplicationMap (std::istringstream& in);
    ApplicationInfo* lookup (std::string appName);
    int nProcesses ();
    void add (std::string name, int l, int n);
    void write (std::ostringstream& out);
  };

}

#define MUSIC_APPLICATION_MAP_HH
#endif

/*
 *  This file is part of MUSIC.
 *  Copyright (C) 2007 CSC, KTH
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

#ifndef MUSIC_PARSE_HH

#include <string>
#include <vector>
#include <istream>
#include <music/application_map.hh>

namespace MUSIC {
  
  class parser {
    std::istream* in;
    void parse_string (std::ostringstream& arg, char delim);
  public:
    parser (std::string s);
    bool eof () { return in->eof (); }
    void ignore_whitespace ();
    std::string next_arg ();
  };

  char ** parse_args (std::string cmd,
		      std::string args);
}

#define MUSIC_PARSE_HH
#endif

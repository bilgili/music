/* This file is part of the skol suite.
   Copyright (C) 2005 Mikael Djurfeldt

   The skol suite is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   The skol suite is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with the skol suite; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */


#ifndef DATAFILE_H

#include <string>
#include <fstream>

class datafile : public std::ifstream {
 protected:
  bool at_header_line ();
  bool scan_line (const char* pattern);
 public:
  datafile (std::string filename) : std::ifstream (filename.data ()) { }
  void ignore_line ();
  void ignore_whitespace ();
  bool read (const char*, int& x);
  bool read (const char*, double& x);
  void skip_header ();
};
#define DATAFILE_H
#endif

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

#include "music/parse.hh"

#include <sstream>
#include <vector>

namespace MUSIC {

  parser::parser (std::string s)
    : in (new std::istringstream (s))
  {
  
  }

  void
  parser::ignore_whitespace ()
  {
    while (isspace (in->peek ()))
      in->ignore ();
  }

  void
  parser::parse_string (std::ostringstream& arg, char delim)
  {
    while (true)
      {
	char c;
	switch (c = in->get ())
	  {
	  case '\'':
	  case '"':
	    if (c == delim)
	      break;
	  default:
	    arg << c;
	    continue;
	  case '\\':
	    arg << (char) in->get ();
	    continue;
	  case EOF:
	    //*fixme* error?
	    break;
	  }
	break;
      }  
  }

  std::string
  parser::next_arg ()
  {
    std::ostringstream arg;
    while (true)
      {
	char c;
	switch (c = in->get ())
	  {
	  default:
	    arg << c;
	    continue;
	  case '\\':
	    arg << (char) in->get ();
	    continue;
	  case '\'':
	  case '"':
	    parse_string (arg, c);
	    continue;
	  case ' ':
	  case '\t':
	  case EOF:
	    break;
	  }
	break;
      }
    return arg.str ();
  }

  char **
  parse_args (std::string cmd,
	      std::string argstring,
	      int* argc)
  {
    parser in (argstring);
    std::vector<std::string> args;
    args.push_back (cmd);
    in.ignore_whitespace ();

    while (! in.eof ())
      args.push_back (in.next_arg ());

    int nargs = args.size ();
    char** result = new char*[nargs + 1];
    for (int i = 0; i < nargs; ++i)
      {
	int len = args[i].length ();
	result[i] = new char[len + 1];
	args[i].copy (result[i], len);
	result[i][len] = '\0';
      }
    result[nargs] = 0;
    *argc = nargs;
    return result;
  }

}

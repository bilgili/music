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


#include <iostream>
#include <limits>
#include <cctype>

#include "datafile.h"

using std::numeric_limits;

bool
datafile::at_header_line ()
{
  return peek () == '#';
}

void
datafile::ignore_line ()
{
  ignore (numeric_limits<int>::max (), '\n');
}

void
datafile::ignore_whitespace ()
{
  while (isspace (peek ()))
    ignore ();
}

bool
datafile::scan_line (const char* pattern)
{
  const char* p = pattern;
  while (*p != '\0')
    {
      if (peek () == '\n')
	{
	  ignore ();
	  return false;
	}
      if (peek () == *p)
	++p;
      else
	p = pattern;
      ignore ();
    }
  return true;
}

bool
datafile::read (const char* pattern, int& x)
{
  seekg (0);
  while (at_header_line ())
    if (scan_line (pattern))
      {
	*this >> x;
	return true;
      }
  return false;
}

bool
datafile::read (const char* pattern, double& x)
{
  seekg (0);
  while (at_header_line ())
    if (scan_line (pattern))
      {
	*this >> x;
	return true;
      }
  return false;
}

void
datafile::skip_header ()
{
  ignore_whitespace ();
  while (at_header_line ())
    ignore_line ();
}

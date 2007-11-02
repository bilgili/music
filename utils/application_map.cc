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

#include "application_map.hh"

#include "rudeconfig/src/config.h"
#include <iostream>


// This is where we parse the configuration file
// *fixme* Should check here that obligatory parameters exists
application_map::application_map (std::istream* config_file)
{
  rude::Config* cfile = new rude::Config ();
  cfile->load (*config_file);
  MUSIC::configuration* def_config = 0;
  int n_sections = cfile->getNumSections ();
  for (int s = 0; s < n_sections; ++s)
    {
      const char* name = cfile->getSectionNameAt (s);
      cfile->setSection (name);
      MUSIC::configuration* config
	= new MUSIC::configuration (s - 1, def_config);

      int n_members = cfile->getNumDataMembers ();
      for (int m = 0; m < n_members; ++m)
	{
	  const char* name = cfile->getDataNameAt (m);
	  config->insert (name, cfile->getStringValue (name));
	}

      if (s == 0)
	def_config = config;
      else
	configs.insert (std::make_pair (name, config));
    }
}


MUSIC::configuration*
application_map::configuration_for_rank (int rank)
{
  int n = configs.size ();
  int rank_end = 0;
  std::map<std::string, MUSIC::configuration*>::iterator config;
  config = configs.begin ();
  while (true)
    {
      int np;
      config->second->lookup ("np", &np);
      rank_end += np;
      //*fixme* better error checking here
      if (rank < rank_end)
	break;
      ++config;
    }
  return config->second;
}

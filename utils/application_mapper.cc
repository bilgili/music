/*
 *  This file is part of MUSIC.
 *  Copyright (C) 2007, 2008 INCF
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

//#define MUSIC_DEBUG 1

#include "music/error.hh"
#include "music/debug.hh"

#include "application_mapper.hh"

namespace MUSIC {

  // This is where we parse the configuration file
  // *fixme* Should check here that obligatory parameters exists
  application_mapper::application_mapper (std::istream* config_file, int rank)
  {
    rude::Config* cfile = new rude::Config ();
    cfile->load (*config_file);

    map_sections (cfile);
    map_applications ();
    select_application (rank);
    map_connectivity (cfile);
  }


  void
  application_mapper::map_sections (rude::Config* cfile)
  {
    MUSIC::configuration* def_config = 0;
    int n_sections = cfile->getNumSections ();
    for (int s = 0; s < n_sections; ++s)
      {
	const char* name = cfile->getSectionNameAt (s);
	cfile->setSection (name);
	MUSIC::configuration* config
	  = new MUSIC::configuration (name, s - 1, def_config);

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


  void
  application_mapper::map_applications ()
  {
    _applications = new application_map ();
    int leader = 0;
    std::map<std::string, MUSIC::configuration*>::iterator config;
    for (config = configs.begin (); config != configs.end (); ++config)
      {
	int np;
	config->second->lookup ("np", &np);
	_applications->add (config->first, leader, np);
	leader += np;
      }
  }


  void
  application_mapper::select_application (int rank)
  {
    int rank_end = 0;
    application_map::iterator ai;
    for (ai = _applications->begin (); ai != _applications->end (); ++ai)
      {
	rank_end += ai->n_proc ();
	if (rank < rank_end)
	  {
	    selected_name = ai->name ();
	    MUSIC_LOG ("rank " << rank << " mapped as " << selected_name);
	    return;
	  }
      }
    error ("internal error in application_mapper::select_application");
  }


  void
  application_mapper::map_connectivity (rude::Config* cfile)
  {
    _connectivity_map = new connectivity ();
    int n_sections = cfile->getNumSections ();
    for (int s = 0; s < n_sections; ++s)
      {
	std::string sec_name (cfile->getSectionNameAt (s));
	cfile->setSection (sec_name.c_str ());

	int n_connections = cfile->getNumSourceDestMembers ();
	for (int c = 0; c < n_connections; ++c)
	  {
	    std::string sender_app (cfile->getSrcAppAt (c));
	    std::string sender_port (cfile->getSrcObjAt (c));
	    std::string receiver_app (cfile->getDestAppAt (c));
	    std::string receiver_port (cfile->getDestObjAt (c));
	    std::string width (cfile->getWidthAt (c));
	  
	    if (sender_app == "")
	      if (sec_name == "")
		error ("sender application not specified for output port " + sender_port);
	      else
		sender_app = sec_name;
	    if (receiver_app == "")
	      if (sec_name == "")
		error ("receiver application not specified for input port " + receiver_port);
	      else
		receiver_app = sec_name;
	    if (sender_app == receiver_app)
	      error ("port " + sender_port + " of application " + sender_app + " connected to the same application");
	  
	    connectivity_info::port_direction dir;
	    application_info* remote_info;
	    if (selected_name == sender_app)
	      {
		dir = connectivity_info::OUTPUT;
		remote_info = _applications->lookup (receiver_app);
	      }
	    else if (selected_name == receiver_app)
	      {
		dir = connectivity_info::INPUT;
		remote_info = _applications->lookup (sender_app);
	      }
	    else
	      continue;
	    int w;
	    if (width == "")
	      w = connectivity_info::NO_WIDTH;
	    else
	      { //*fixme*
		std::istringstream ws (width);
		if (!(ws >> w))
		  error ("could not interpret width");
	      }
	    _connectivity_map->add (dir == connectivity_info::OUTPUT
				    ? sender_port
				    : receiver_port,
				    dir,
				    w,
				    receiver_app,
				    receiver_port,
				    remote_info->leader (),
				    remote_info->n_proc ());
	  }
      }
  }


  MUSIC::configuration*
  application_mapper::config ()
  {
    configuration* config = configs[selected_name];
    config->set_applications (_applications);
    config->set_connectivity_map (_connectivity_map);
    return config;
  }

}

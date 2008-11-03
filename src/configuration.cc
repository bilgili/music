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

extern "C" {
#include <stdlib.h>
}

#include "music/configuration.hh"
#include "music/ioutils.hh"
#include "music/error.hh"

namespace MUSIC {
  
  const char* const configuration::config_env_var_name = "_MUSIC_CONFIG_";

  
  configuration::configuration (std::string name, int color, configuration* def)
    : _application_name (name), _color (color), default_config (def)
  {
    
  }

  
  configuration::configuration ()
    : default_config (0)
  {
    int n_vars;
    char* config_str = getenv (config_env_var_name);
    if (config_str == NULL)
      {
	_launched_by_music = false;
	_applications = new application_map ();
	_connectivity_map = new connectivity ();
      }
    else
      {
	_launched_by_music = true;
	std::istringstream env (config_str);
	_application_name = ioutils::read (env);
	env.ignore ();
	env >> _color;
	env.ignore ();
	_applications = new application_map (env);
	env.ignore ();
	_connectivity_map = new connectivity (env);
	// parse config string
	while (!env.eof ())
	  {
	    env.ignore ();
	    std::string name = ioutils::read (env, '=');
	    env.ignore ();
	    insert (name, ioutils::read (env));
	  }
      }
  }


  configuration::~configuration ()
  {
    delete _connectivity_map;
    delete _applications;
  }

  
  void
  configuration::write (std::ostringstream& env, configuration* mask)
  {
    std::map<std::string, std::string>::iterator pos;
    for (pos = dict.begin (); pos != dict.end (); ++pos)
      {
	std::string name = pos->first;
	if (!(mask && mask->lookup (name)))
	  {
	    env << ':' << name << '=';
	    ioutils::write (env, pos->second);
	  }
      }
  }

  
  void
  configuration::write_env ()
  {
    //*fixme* generally make lexical structure more strict and make
    //sure that everything written can be consistently read back
    std::ostringstream env;
    env << _application_name << ':' << _color << ':';
    _applications->write (env);
    env << ':';
    _connectivity_map->write (env);
    write (env, 0);
    default_config->write (env, this);
    setenv (config_env_var_name, env.str ().c_str (), 1);
  }

  
  bool
  configuration::lookup (std::string name)
  {
    return dict.find (name) != dict.end ();
  }

  
  bool
  configuration::lookup (std::string name, std::string* result)
  {
    std::map<std::string, std::string>::iterator pos = dict.find (name);
    if (pos != dict.end ())
      {
	*result = pos->second;
	return true;
      }
    else
      return default_config && default_config->lookup (name, result);
  }

  
  bool
  configuration::lookup (std::string name, int* result)
  {
    std::map<std::string, std::string>::iterator pos = dict.find (name);
    if (pos != dict.end ())
      {
	std::istringstream iss (pos->second);
	if ((iss >> *result).fail ())
	  {
	    std::ostringstream oss;
	    oss << "var " << name << " given wrong type (" << pos->second
		<< "; expected int) in config file";
	    error (oss.str ());
	  }
	return true;
      }
    else
      return default_config && default_config->lookup (name, result);
  }

  
  bool
  configuration::lookup (std::string name, double* result)
  {
    std::map<std::string, std::string>::iterator pos = dict.find (name);
    if (pos != dict.end ())
      {
	std::istringstream iss (pos->second);
	if ((iss >> *result).fail ())
	  {
	    std::ostringstream oss;
	    oss << "var " << name << " given wrong type (" << pos->second
		<< "; expected double) in config file";
	    error (oss.str ());
	  }
	return true;
      }
    else
      return default_config && default_config->lookup (name, result);
  }

  
  void
  configuration::insert (std::string name, std::string value)
  {
    dict.insert (std::make_pair (name, value));
  }

  
  application_map*
  configuration::applications ()
  {
    return _applications;
  }

  
  void
  configuration::set_applications (application_map* a)
  {
    _applications = a;
  }

  
  connectivity*
  configuration::connectivity_map ()
  {
    return _connectivity_map;
  }


  void
  configuration::set_connectivity_map (connectivity* c)
  {
    _connectivity_map = c;
  }

}

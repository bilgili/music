/*
 *  This file is part of MUSIC.
 *  Copyright (C) 2007, 2008 CSC, KTH
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

#include <sstream>

extern "C" {
#include <stdlib.h>
}

#include "music/configuration.hh"
#include "music/error.hh"

namespace MUSIC {
  
  const char* const configuration::config_env_var_name = "_MUSIC_CONFIG_";

  
  configuration::configuration (int color, configuration* def)
    : _color (color), default_config (def)
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
      }
    else
      {
	_launched_by_music = true;
	std::istringstream env (config_str);
	env >> _color;
	// parse config string
	env.ignore (); // ignore initial colon
	while (!env.eof ())
	  {
	    const int maxsize = 80;
	    char* name = new char[maxsize];
	    env.get (name, maxsize, '=');
	    env.ignore ();
	    std::ostringstream value;
	    while (true)
	      {
		int c = env.get ();
		switch (c)
		  {
		  case '\\':
		    value << (char) env.get ();
		    continue;
		  default:
		    value << (char) c;
		    continue;
		  case ':':
		  case EOF:
		    break;
		  }
		break;
	      }
	    insert (name, value.str ());
	  }
      }
  }

  
  void
  configuration::tap (std::ostringstream& env, configuration* mask)
  {
    std::map<std::string, std::string>::iterator pos;
    for (pos = dict.begin (); pos != dict.end (); ++pos)
      {
	std::string name = pos->first;
	if (!(mask && mask->lookup (name)))
	  {
	    env << ':' << name << '=';
	    std::istringstream value (pos->second);
	    while (true)
	      {
		int c;
		switch (c = value.get ())
		  {
		  case '\\':
		  case ':':
		    env << '\\';
		  default:
		    env << (char) c;
		    continue;
		  case EOF:
		    break;
		  }
		break;
	      }
	  }
      }
  }

  
  void
  configuration::write_env ()
  {
    std::ostringstream env;
    env << _color;
    tap (env, 0);
    default_config->tap (env, this);
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

  connectivity*
  configuration::connectivity_map ()
  {
    return _connectivity_map;
  }

}

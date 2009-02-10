/*
 *  This file is part of MUSIC.
 *  Copyright (C) 2007, 2008, 2009 INCF
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
#include "music/debug.hh" // Must be included first on BG/L

extern "C" {
#include <stdlib.h>
}

#include "music/configuration.hh"
#include "music/ioutils.hh"
#include "music/error.hh"

namespace MUSIC {
  
  const char* const Configuration::configEnvVarName = "_MUSIC_CONFIG_";

  
  Configuration::Configuration (std::string name, int color, Configuration* def)
    : _applicationName (name), _color (color), defaultConfig (def)
  {
    
  }

  
  Configuration::Configuration ()
    : defaultConfig (0)
  {
    int nVars;
    char* configStr = getenv (configEnvVarName);
    MUSIC_LOG0 ("config: " << configStr);
    if (configStr == NULL)
      {
	_launchedByMusic = false;
	_applications = new ApplicationMap ();
	_connectivityMap = new Connectivity ();
      }
    else
      {
	_launchedByMusic = true;
	std::istringstream env (configStr);
	_applicationName = IOUtils::read (env);
	env.ignore ();
	env >> _color;
	env.ignore ();
	_applications = new ApplicationMap (env);
	env.ignore ();
	_connectivityMap = new Connectivity (env);
	// parse config string
	while (!env.eof ())
	  {
	    env.ignore ();
	    std::string name = IOUtils::read (env, '=');
	    env.ignore ();
	    insert (name, IOUtils::read (env));
	  }
      }
  }


  Configuration::~Configuration ()
  {
    delete _connectivityMap;
    delete _applications;
  }

  
  void
  Configuration::write (std::ostringstream& env, Configuration* mask)
  {
    std::map<std::string, std::string>::iterator pos;
    for (pos = dict.begin (); pos != dict.end (); ++pos)
      {
	std::string name = pos->first;
	if (!(mask && mask->lookup (name)))
	  {
	    env << ':' << name << '=';
	    IOUtils::write (env, pos->second);
	  }
      }
  }

  
  void
  Configuration::writeEnv ()
  {
    //*fixme* generally make lexical structure more strict and make
    //sure that everything written can be consistently read back
    std::ostringstream env;
    env << _applicationName << ':' << _color << ':';
    _applications->write (env);
    env << ':';
    _connectivityMap->write (env);
    write (env, 0);
    defaultConfig->write (env, this);
    setenv (configEnvVarName, env.str ().c_str (), 1);
  }

  
  bool
  Configuration::lookup (std::string name)
  {
    return dict.find (name) != dict.end ();
  }

  
  bool
  Configuration::lookup (std::string name, std::string* result)
  {
    std::map<std::string, std::string>::iterator pos = dict.find (name);
    if (pos != dict.end ())
      {
	*result = pos->second;
	return true;
      }
    else
      return defaultConfig && defaultConfig->lookup (name, result);
  }

  
  bool
  Configuration::lookup (std::string name, int* result)
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
      return defaultConfig && defaultConfig->lookup (name, result);
  }

  
  bool
  Configuration::lookup (std::string name, double* result)
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
      return defaultConfig && defaultConfig->lookup (name, result);
  }

  
  void
  Configuration::insert (std::string name, std::string value)
  {
    dict.insert (std::make_pair (name, value));
  }

  
  ApplicationMap*
  Configuration::applications ()
  {
    return _applications;
  }

  
  void
  Configuration::setApplications (ApplicationMap* a)
  {
    _applications = a;
  }

  
  Connectivity*
  Configuration::connectivityMap ()
  {
    return _connectivityMap;
  }


  void
  Configuration::setConnectivityMap (Connectivity* c)
  {
    _connectivityMap = c;
  }

}

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

#ifndef MUSIC_CONFIGURATION_HH

#include <string>
#include <map>

namespace MUSIC {

  class configuration {
  private:
    static const char* const config_env_var_name;
    int _color;
    configuration* default_config;
    std::map<std::string, std::string> dict;
    void tap (std::ostringstream& env, configuration* mask);
  public:
    configuration ();
    configuration (int color, configuration* def);
    void write_env ();
    int color () { return _color; };
    bool lookup (std::string name);
    bool lookup (std::string name, std::string* result);
    bool lookup (std::string name, int* result);
    bool lookup (std::string name, double* result);
    void insert (std::string name, std::string value);
  };

}

#define MUSIC_CONFIGURATION_HH
#endif

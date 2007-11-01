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

#ifndef MUSIC_APPLICATION_MAP_HH

#include <istream>
#include <map>

#include <music/configuration.hh>

class application_map {
  std::map<std::string, MUSIC::configuration*> configs;
public:
  application_map (std::istream* config_file);
  MUSIC::configuration* configuration_for_rank (int rank);
};

#define MUSIC_APPLICATION_MAP_HH
#endif

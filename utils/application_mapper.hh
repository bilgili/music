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

#ifndef MUSIC_APPLICATION_MAPPER_HH

#include <istream>
#include <map>

#include "rudeconfig/src/config.h"

#include <music/configuration.hh>

namespace MUSIC {

  class application_mapper {
    std::map<std::string, MUSIC::configuration*> configs;
    application_map* _applications;
    connectivity* _connectivity_map;
    std::string selected_name;
    void map_sections (rude::Config* cfile);
    void map_applications ();
    void select_application (int rank);
    void map_connectivity (rude::Config* cfile);
  public:
    application_mapper (std::istream* config_file, int rank);
    MUSIC::configuration* config ();
  };

}

#define MUSIC_APPLICATION_MAPPER_HH
#endif

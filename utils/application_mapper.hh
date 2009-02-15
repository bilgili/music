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

#ifndef MUSIC_APPLICATION_MAPPER_HH

#include <istream>
#include <map>

#include "rudeconfig/src/config.h"

#include <music/configuration.hh>

namespace MUSIC {

  class ApplicationMapper {
    std::map<std::string, MUSIC::Configuration*> configs;
    ApplicationMap* applications_;
    Connectivity* connectivityMap_;
    std::string selectedName;
    void mapSections (rude::Config* cfile);
    void mapApplications ();
    void selectApplication (int rank);
    void mapConnectivity (rude::Config* cfile);
  public:
    ApplicationMapper (std::istream* configFile, int rank);
    MUSIC::Configuration* config ();
  };

}

#define MUSIC_APPLICATION_MAPPER_HH
#endif

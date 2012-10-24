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

#ifndef MUSIC_CONFIGURATION_HH

#include "music/music-config.hh"

#include <string>
#include <map>

#include "music/application_map.hh"
#include "music/connectivity.hh"

namespace MUSIC {
//#define __bgp__
  class Configuration {
  private:
    static const char* const configEnvVarName;
    /* remedius
     * name of the file that contains mapping of the ranks to applications
     * and according configEnvVarName.
     */
    static const char* const mapFileName;
    bool launchedByMusic_;
    std::string applicationName_;
    int color_;
    int leader_;
    Configuration* defaultConfig;
    ApplicationMap* applications_;
    Connectivity* connectivityMap_;

    std::map<std::string, std::string> dict;
    void write (std::ostringstream& env, Configuration* mask);
#if MUSIC_USE_MPI
    void getEnvFromFile (char *app_name, std::string* result);
    void getEnv (char *app_name, std::string* result);

    /* remedius
     * Parses <map_file> in order to read configEnvVarName(<result>) that suites current rank.
     */
    void parseMapFile(char *app_name, std::string map_file, std::string *result);
#endif
  public:
#if MUSIC_USE_MPI
    Configuration (char *app_name);
#endif
    Configuration (std::string name, int color, Configuration* def);
    ~Configuration ();
    bool launchedByMusic () { return launchedByMusic_; }
    void writeEnv ();
    int color () { return color_; }
    int leader () { return leader_; }
    bool lookup (std::string name);
    bool lookup (std::string name, std::string* result);
    bool lookup (std::string name, int* result);
    bool lookup (std::string name, double* result);
    void insert (std::string name, std::string value);
    std::string ApplicationName();
    ApplicationMap* applications ();
    void setApplications (ApplicationMap*);
    Connectivity* connectivityMap ();
    void setConnectivityMap (Connectivity* c);
  };

}

#define MUSIC_CONFIGURATION_HH
#endif

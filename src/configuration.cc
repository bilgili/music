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

#include "music/configuration.hh" // Must be included first on BG/L
#if MUSIC_USE_MPI
#include <mpi.h>
#endif
extern "C" {
#include <stdlib.h>
}
#include "music/ioutils.hh"
#include "music/error.hh"
#include <iostream>
#include <fstream>
namespace MUSIC {

const char* const Configuration::configEnvVarName = "_MUSIC_CONFIG_";
const char* const Configuration::mapFileName = "music.map";

Configuration::Configuration (std::string name, int color, Configuration* def)
: applicationName_ (name), color_ (color), defaultConfig (def)
{

}
#if MUSIC_USE_MPI
Configuration::Configuration (char *app_name)
: defaultConfig (0)
{
	std::string configStr;
	/* remedius
	 * getenv system call was replaced with local method getEnv call
	 * in order to support reading configStr from the file (BG/P).
	 */
	getEnv (app_name, &configStr);
	MUSIC_LOG0 ("config: " << configStr);
	if (configStr.length() == 0)
	{
		launchedByMusic_ = false;
		applications_ = new ApplicationMap ();
		connectivityMap_ = new Connectivity ();
	}
	else
	{
		launchedByMusic_ = true;
		std::istringstream env (configStr);

		applicationName_ = IOUtils::read (env);
		env.ignore ();
		env >> color_;
		env.ignore ();
		applications_ = new ApplicationMap (env, color_);
		env.ignore ();
		connectivityMap_ = new Connectivity (env, applications_->LeaderIdHook());
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
#endif

Configuration::~Configuration ()
{
	delete connectivityMap_;
	delete applications_;
}

#if MUSIC_USE_MPI
void
Configuration::getEnv(char *app_name,  std::string* result)
{
#if defined(__bgp__)
	int rank = MPI::COMM_WORLD.Get_rank ();
	std::ifstream mapFile;
	char* buffer;
	int size = 0;
	// Rank #0 is reading a file and broadcast it to each rank in the launch
	if(rank == 0){
		mapFile.open(mapFileName);
		if (!mapFile.is_open())
		{
			std::ostringstream oss;
			oss << "File <music.map> is not found. To generate the file run MUSIC with -f option.";
			error (oss.str ());
		}

		size = mapFile.tellg();
		mapFile.seekg( 0, std::ios_base::end );
		long cur_pos = mapFile.tellg();
		size = cur_pos - size;
		mapFile.seekg( 0, std::ios_base::beg );
	}
	// first broadcast the size of the file
	MPI::COMM_WORLD.Bcast(&size, 1,  MPI::INT, 0);
	buffer = new char[size];

	if(rank == 0)
		mapFile.read ( buffer, size );
	// then broadcast the file but itself
	MPI::COMM_WORLD.Bcast(buffer, size,  MPI::BYTE, 0);
	parseMapFile(app_name, std::string(buffer, size), result);
	if(rank == 0)
		mapFile.close();
	delete[] buffer;
#else
	result->assign(getenv(configEnvVarName));
#endif

}
#endif
/* remedius
 * Each rank is responsible for parsing <map_file> and
 * retrieving according environment variable value (<result>)
 */


#if defined(__bgp__)
void
Configuration::parseMapFile(char *app_name, std::string map_file, std::string *result)
{
	int pos, sLen, color;
	std::string applicationName;
	pos = 0;
	sLen = map_file.length();
	do{
		size_t occ_e = map_file.find_first_of ("\n", pos);
		std::istringstream env (map_file.substr(pos,occ_e));
		applicationName = IOUtils::read (env);
		env.ignore ();
		env >> color;
		if(!applicationName.compare(app_name)){
			if(result->length() > 0){
				std::ostringstream oss;
				oss << "Multiple applications with the same name are listed in <music.map> file." << std::endl;
				error (oss.str ());
			}
			*result =map_file.substr(pos,occ_e);
			//return;
		}
		pos = occ_e+1;
	}while(pos < sLen);
	if(result->length() == 0){
		std::ostringstream oss;
		oss << "There is a mismatch between the information in the file <music.map> and the given application color." << std::endl
				<< "Try to generate the file (run MUSIC with -f option) again.";
		error (oss.str ());
	}
}
#endif

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
	std::ostringstream env;
	env << applicationName_ << ':' << color_ << ':';
	applications_->write (env);
	env << ':';
	connectivityMap_->write (env);
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

std::string Configuration::ApplicationName()
{
	return applicationName_;
}

void
Configuration::insert (std::string name, std::string value)
{

	dict.insert (std::make_pair (name, value));
}


ApplicationMap*
Configuration::applications ()
{
	return applications_;
}


void
Configuration::setApplications (ApplicationMap* a)
{
	applications_ = a;
}


Connectivity*
Configuration::connectivityMap ()
{
	return connectivityMap_;
}


void
Configuration::setConnectivityMap (Connectivity* c)
{
	connectivityMap_ = c;
}

}

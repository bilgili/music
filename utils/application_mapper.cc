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

#include "music/error.hh"
#include "music/debug.hh"
#include <iostream>
#include <stdlib.h>
#include <algorithm>

#include "application_mapper.hh"

namespace MUSIC {

  // This is where we parse the configuration file

  // NOTE: Could check here that obligatory parameters exists
  ApplicationMapper::ApplicationMapper (std::istream* configFile, int rank)
  {
    cfile = new rude::Config ();
    if(cfile->load (*configFile))
    {
    	mapSections (cfile);
    	mapApplications ();
    	selectApplication (rank);
    	mapConnectivity (configs[selectedApp]->ApplicationName());
    }
    else{
    	error0("Configuration file load: " + std::string(cfile->getError()));
    }
  }


  void
  ApplicationMapper::mapSections (rude::Config* cfile)
  {
    MUSIC::Configuration* defConfig = 0;
    int nSections = cfile->getNumSections ();
    for (int s = 0; s < nSections; ++s)
      {
	const char* name = cfile->getSectionNameAt (s);
	cfile->setSection (name);
	MUSIC::Configuration* config
	  = new MUSIC::Configuration (name, s - 1, defConfig);

	int nMembers = cfile->getNumDataMembers ();
	for (int m = 0; m < nMembers; ++m)
	  {
	    const char* name = cfile->getDataNameAt (m);
	    config->insert (name, cfile->getStringValue (name));
	  }

	if (s == 0)
	  defConfig = config;
	else
	  configs.insert (std::make_pair (s - 1, config));
      }
  }


  void
  ApplicationMapper::mapApplications ()
  {
    applications_ = new ApplicationMap ();
    int leader = 0;
    std::map<int, MUSIC::Configuration*>::iterator config;
    int i = 0;
    for (config = configs.begin (); config != configs.end (); ++config, ++i)
      {
	int np;
	config->second->lookup ("np", &np);
	applications_->add (config->second->ApplicationName(), leader, np, config->first);
	leader += np;
      }
  }


  void
  ApplicationMapper::selectApplication (int rank)
  {
#if MUSIC_USE_MPI
    int rankEnd = 0;
    ApplicationMap::iterator ai;
    for (ai = applications_->begin (); ai != applications_->end (); ++ai)
      {
	rankEnd += ai->nProc ();
	if (rank < rankEnd)
	  {
	    selectedApp = ai->color();
	    MUSIC_LOG ("rank " << rank << " mapped as " << ai->name());
	    return;
	  }
      }
#endif

    // postpone error reporting to setup
    // NOTE: consider doing error checking here

    // for now, choose the first application
    selectedApp= applications_->begin ()->color();

  }


  void
  ApplicationMapper::mapConnectivity (std::string thisName)
  {
    std::map<std::string, int> receiverPortCodes;
    int nextPortCode = 0;
    connectivityMap_ = new Connectivity ();
    int nSections = cfile->getNumSections ();
    for (int s = 0; s < nSections; ++s)
      {
	std::string secName (cfile->getSectionNameAt (s));
	cfile->setSection (secName.c_str ());

	int nConnections = cfile->getNumSourceDestMembers ();
	for (int c = 0; c < nConnections; ++c)
	  {
	    std::string senderApp (cfile->getSrcAppAt (c));
	    std::string senderPort (cfile->getSrcObjAt (c));
	    std::string receiverApp (cfile->getDestAppAt (c));
	    std::string receiverPort (cfile->getDestObjAt (c));
	    std::string width (cfile->getWidthAt (c));
	    std::string commType(cfile->getCommTypeAt(c));
	    std::string procMethod(cfile->getProcMethodAt(c));

	    if (senderApp == "")
	      {
		if (secName == "")
		  error ("sender application not specified for output port " + senderPort);
		else
		  senderApp = secName;
	      }
	    if (receiverApp == "")
	      {
		if (secName == "")
		  error ("receiver application not specified for input port " + receiverPort);
		else
		  receiverApp = secName;
	      }
	    if (senderApp == receiverApp)
	      error ("port " + senderPort + " of application " + senderApp + " connected to the same application");


	    /* remedius
	     *  Since two runtime configuration options were added: communication type (<commType>) and
	     *  processing method (<procMethod>), there is a basic check for the match of the reserved words to these options.
	     *  Communication type option can be either *point-to-point* or *collective* written in any case letters.
	     *  Processing method can be either *tree* or *table* also written in any case letters.
	     */
	    std::transform(commType.begin(), commType.end(), commType.begin(), ::tolower);
	    std::transform(procMethod.begin(), procMethod.end(), procMethod.begin(), ::tolower);

	    if(commType.length() > 0 && commType.compare("collective") && commType.compare("point-to-point"))
	    	error ("communication type " + commType + " is not supported");

	    if(procMethod.length() > 0 && procMethod.compare("table") && procMethod.compare("tree"))
	    	    	error ("processing method " + procMethod + " is not supported");

	    // Generate a unique "port code" for each receiver port
	    // name.  This will later be used during temporal
	    // negotiation since it easier to communicate integers,
	    // which have constant size, than strings.
	    //
	    // NOTE: This code must be executed in the same order in
	    // all MPI processes.
	    std::string receiverPortFullName = receiverApp + "." + receiverPort;
	    std::map<std::string, int>::iterator pos
	      = receiverPortCodes.find (receiverPortFullName);
	    int portCode;
	    if (pos == receiverPortCodes.end ())
	      {
		portCode = nextPortCode++;
		receiverPortCodes.insert (std::make_pair (receiverPortFullName,
							  portCode));
	      }
	    else
	      portCode = pos->second;

	    ConnectivityInfo::PortDirection dir;
	    ApplicationInfo* remoteInfo;
	    if (thisName == senderApp)
	      {
		dir = ConnectivityInfo::OUTPUT;
		remoteInfo = applications_->lookup (receiverApp);
	      }
	    else if (thisName == receiverApp)
	      {
		dir = ConnectivityInfo::INPUT;
		remoteInfo = applications_->lookup (senderApp);
	      }
	    else
	      continue;
	    int w;
	    if (width == "")
	      w = ConnectivityInfo::NO_WIDTH;
	    else
	      {
		std::istringstream ws (width);
		if (!(ws >> w))
		  error ("could not interpret width");
	      }
	    /* remedius
	     * The default communication type is *point-to-point*
	     */
	    int iCommType;
	    if(commType.length()==0 || !commType.compare("point-to-point")){
	    	iCommType = ConnectorInfo::POINTTOPOINT;
	    }

	    else{
	    	iCommType = ConnectorInfo::COLLECTIVE;
	    }
	    /* remedius
	     * The default processing method is *tree*
	     */
	    int iProcMethod;
	    if(procMethod.length()==0 || !procMethod.compare("tree")){
	    	iProcMethod = ConnectorInfo::TREE;
	    }

	    else{
	    	iProcMethod = ConnectorInfo::TABLE;
	    }
	    connectivityMap_->add (dir == ConnectivityInfo::OUTPUT
				   ? senderPort
				   : receiverPort,
				   dir,
				   w,
				   receiverApp,
				   receiverPort,
				   portCode,
				   remoteInfo->leader (),
				   remoteInfo->nProc (),
				   iCommType,
				   iProcMethod);
	  }
      }

  }


  MUSIC::Configuration*
  ApplicationMapper::config ()
  {
    return config (selectedApp);
  }

  
  MUSIC::Configuration*
  ApplicationMapper::config (int appId)
  {
    Configuration* config = configs[appId];
    config->setApplications (applications_);
    config->setConnectivityMap (connectivityMap_);
    return config;
  }

}

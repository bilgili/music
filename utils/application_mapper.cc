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

#include "music/error.hh"
#include "music/debug.hh"

#include "application_mapper.hh"

namespace MUSIC {

  // This is where we parse the configuration file
  // *fixme* Should check here that obligatory parameters exists
  ApplicationMapper::ApplicationMapper (std::istream* configFile, int rank)
  {
    rude::Config* cfile = new rude::Config ();
    cfile->load (*configFile);

    mapSections (cfile);
    mapApplications ();
    selectApplication (rank);
    mapConnectivity (cfile);
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
	  configs.insert (std::make_pair (name, config));
      }
  }


  void
  ApplicationMapper::mapApplications ()
  {
    applications_ = new ApplicationMap ();
    int leader = 0;
    std::map<std::string, MUSIC::Configuration*>::iterator config;
    for (config = configs.begin (); config != configs.end (); ++config)
      {
	int np;
	config->second->lookup ("np", &np);
	applications_->add (config->first, leader, np);
	leader += np;
      }
  }


  void
  ApplicationMapper::selectApplication (int rank)
  {
    int rankEnd = 0;
    ApplicationMap::iterator ai;
    for (ai = applications_->begin (); ai != applications_->end (); ++ai)
      {
	rankEnd += ai->nProc ();
	if (rank < rankEnd)
	  {
	    selectedName = ai->name ();
	    MUSIC_LOG ("rank " << rank << " mapped as " << selectedName);
	    return;
	  }
      }
    error ("internal error in ApplicationMapper::selectApplication");
  }


  void
  ApplicationMapper::mapConnectivity (rude::Config* cfile)
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

	    if (senderApp == "")
	      if (secName == "")
		error ("sender application not specified for output port " + senderPort);
	      else
		senderApp = secName;
	    if (receiverApp == "")
	      if (secName == "")
		error ("receiver application not specified for input port " + receiverPort);
	      else
		receiverApp = secName;
	    if (senderApp == receiverApp)
	      error ("port " + senderPort + " of application " + senderApp + " connected to the same application");

	    // Generate a unique "port code" for each receiver port
	    // name.  This will later be used during temporal
	    // negotiation since it easier to communicate integers,
	    // which have constant size, than strings.
	    //
	    // NOTE: This code must be executed in the same order in
	    // all MPI processes.
	    std::map<std::string, int>::iterator pos
	      = receiverPortCodes.find (receiverPort);
	    int portCode;
	    if (pos == receiverPortCodes.end ())
	      {
		portCode = nextPortCode++;
		receiverPortCodes.insert (std::make_pair (receiverPort,
							  portCode));
	      }
	    else
	      portCode = pos->second;

	    ConnectivityInfo::PortDirection dir;
	    ApplicationInfo* remoteInfo;
	    if (selectedName == senderApp)
	      {
		dir = ConnectivityInfo::OUTPUT;
		remoteInfo = applications_->lookup (receiverApp);
	      }
	    else if (selectedName == receiverApp)
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
	      { //*fixme*
		std::istringstream ws (width);
		if (!(ws >> w))
		  error ("could not interpret width");
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
				   remoteInfo->nProc ());
	  }
      }
  }


  MUSIC::Configuration*
  ApplicationMapper::config ()
  {
    Configuration* config = configs[selectedName];
    config->setApplications (applications_);
    config->setConnectivityMap (connectivityMap_);
    return config;
  }

}

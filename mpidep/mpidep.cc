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

#include "config.h"

#include <string>
#include <sstream>
#include <fstream>

extern "C" {
#include <stdlib.h>
}

#include "music/error.hh"

#include "mpidep.hh"

// Implementation-dependent code

#ifdef HAVE_RTS_GET_PERSONALITY
#define BGL
#else
#ifdef HAVE_OMPI_COMM_FREE
#define OPENMPI
#else
#define MPICH
#endif
#endif

#ifdef BGL
#include <rts.h>
#endif

int
getRank (int argc, char *argv[])
{
#ifdef BGL
  BGLPersonality p;
  rts_get_personality (&p, sizeof (p));
  int pid = rts_get_processor_id ();
  unsigned int rank, np;
  rts_rankForCoordinates (p.xCoord, p.yCoord, p.zCoord, pid, &rank, &np);
  return rank;
#endif
#ifdef MPICH
  int rank;
  const std::string rankopt = "-p4rmrank";
  for (int i = argc - 2; i > 0; --i)
    if (argv[i] == rankopt)
      {
	std::istringstream iss (argv[i + 1]);
	iss >> rank;
	return rank;
      }
  return 0;
#endif
#ifdef OPENMPI
  char* vpid = getenv ("OMPI_MCA_ns_nds_vpid");
  if (vpid == NULL)
    vpid = getenv ("OMPI_COMM_WORLD_RANK");
  if (vpid == NULL)
    MUSIC::error ("getRank: unable to determine process rank");
  std::istringstream iss (vpid);
  int rank;
  iss >> rank;
  return rank;
#endif
}


#ifdef MPICH
std::string
getSharedDir ()
{
  std::ostringstream dirname;
  char* musicSharedDir = getenv ("MUSIC_SHARED_DIR");
  if (musicSharedDir)
    dirname << musicSharedDir;
  else
    {
      char* home = getenv ("HOME");
      dirname << home;
    }
  return dirname.str ();
}
#endif


std::istream*
getConfig (int rank, int argc, char** argv)
{
#ifdef MPICH
  std::ostringstream fname;
  fname << getSharedDir () << "/.musicconf";
  std::string confname;
  if (rank == 0)
    {
      std::ofstream f (fname.str ().c_str ());
      confname = argv[1];
      f << confname;
    }
  else
    {
      std::ifstream f (fname.str ().c_str ());
      f >> confname;
    }
  return new std::ifstream (confname.c_str ());
#else
  return new std::ifstream (argv[1]);
#endif
}

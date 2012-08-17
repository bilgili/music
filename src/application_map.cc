/*
 *  This file is part of MUSIC.
 *  Copyright (C) 2008, 2009 INCF
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


#include "music/application_map.hh"
#if MUSIC_USE_MPI
#include <mpi.h>
#endif
#include "music/ioutils.hh"
#include <iostream>
#include <fstream>
namespace MUSIC {

  ApplicationMap::ApplicationMap (std::istringstream& in, int color )
  {
	  std::vector<int> appColor2Leader;
	  int nApp;
	  in >> nApp;
	  /*
	   * app_names.size() is not equal 0 when music.map was used to run MUSIC library
	   * In particular, it's used on BGP machine
	   */
#if MUSIC_USE_MPI
	  appColor2Leader = assignLeaders(nApp, color);
#endif
	  read (in, nApp, appColor2Leader);
  }

  int
  ApplicationMap::nProcesses ()
  {
    int n = 0;
    for (ApplicationMap::iterator i = begin (); i != end (); ++i)
      n += i->nProc ();
    return n;
  }
  
  
  ApplicationInfo*
  ApplicationMap::lookup (std::string appName)
  {
    for (iterator i = begin (); i != end (); ++i)
      {
	if (i->name () == appName)
	  return &*i;
      }
    return 0;
  }
  

  void
  ApplicationMap::add (std::string name, int l, int n, int c)
  {
    push_back (ApplicationInfo (name, l, n, c));
  }

  
  void
  ApplicationMap::write (std::ostringstream& out)
  {
	  out << size ();
	  for (iterator i = begin (); i != end (); ++i)
	  {
		  out << ':';
		  IOUtils::write (out, i->name ());
		  out << ':' << i->nProc ();
	  }
  }
  /* remedius
    * if appName2Leader mapping is known, then this function uses its values,
    * otherwise it assumes the leader number
    * is based on sequential distribution of the ranks among the application:
    * leader+=np
    */
  void
  ApplicationMap::read (std::istringstream& in, int nApp, std::vector<int> appColor2Leader)
  {
	  int leader, aleader;
#ifdef MUSIC_DEBUG
	  /*  for debugging */
	  std::ofstream outfile ("leaders");
#endif
	  leader = 0;
	  bool seq = appColor2Leader.size() == 0;
	  for (int i = 0; i < nApp; ++i)
	  {
		  in.ignore ();
		  std::string name = IOUtils::read (in);
		  in.ignore ();
		  int np;
		  in >> np;
		  aleader = seq ? leader : appColor2Leader[i];
#ifdef MUSIC_DEBUG
		  /*  for debugging */
		  outfile<< name << "\t" << aleader << std::endl;
#endif
		  push_back (ApplicationInfo (name, aleader, np, i));
		  leaderIdHook_[leader] = aleader;
		  leader+=np;
	  }
#ifdef MUSIC_DEBUG
	  	/*  for debugging */
	  	outfile.close();
#endif
  }


#if MUSIC_USE_MPI
  /* remedius
   * map name of the application to its leader based on non sequential distribution of the ranks among the applications:
   * each rank has report to rank# 0 to which application it belongs to,
   * rank#0 decides the leader for each application and distributes this information.
   */
  std::vector<int>
  ApplicationMap::assignLeaders(int nLeaders, int color)
  {
	  std::vector<int> appColor2Leader;
	  int root, gsize, myrank, *rbuf;
	  root = 0;
	  gsize = MPI::COMM_WORLD.Get_size();
	  myrank = MPI::COMM_WORLD.Get_rank();
	  if ( myrank == root)
		  rbuf = new int[gsize];
	  MPI::COMM_WORLD.Gather( &color, 1, MPI_INT, rbuf, 1, MPI_INT, root);
	  int *leaders;
	  leaders = new int[nLeaders];
	  if ( myrank == root) {

		  for(int i = 0; i < nLeaders; ++i)
			  leaders[i] = -1;
		  for(int i = 0; i < gsize; ++i)
			  if( leaders[rbuf[i]] == -1 )
				  leaders[rbuf[i]] = i;

#ifdef MUSIC_DEBUG
		  /*  block for debugging */
		  std::ofstream outfile ("ranks");
		  for(int i = 0; i < nLeaders; ++i){
			  outfile<< i <<":";
			  for(int j = 0; j < gsize; ++j)
				  if(rbuf[j] == i) outfile<< " " << j ;
			  outfile<<std::endl;
		  }
		  outfile<<std::endl;
		  outfile.close();
		  /*end of block */
#endif // MUSIC_DEBUG
	  }
	  MPI::COMM_WORLD.Bcast(leaders, nLeaders, MPI_INT, root);
	  if ( myrank == root) {
		  delete rbuf;
	  }
	  appColor2Leader.insert(appColor2Leader.begin(),leaders,leaders+nLeaders);
	  delete leaders;
	  return appColor2Leader;
  }
#endif
}


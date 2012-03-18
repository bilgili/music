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

#ifndef MUSIC_DEBUG_HH

#ifdef MUSIC_DEBUG

#include <mpi.h> // Must be included first on BG/L
#include <iostream>
// MUSIC_LOGLR:
#include <sstream>
#include <fstream>
#include <iomanip>
extern "C" {
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
}

#define MUSIC_LOG(X) (std::cerr << X << std::endl << std::flush)

#define MUSIC_LOGN(N, X) \
  { if (MPI::COMM_WORLD.Get_rank () == N) MUSIC_LOG (X); }

#define MUSIC_LOG0(X) MUSIC_LOGN (0, X)

#define MUSIC_SLOGR(S, X)				\
  {							\
    (S) << MPI::COMM_WORLD.Get_rank () << ": "		\
	<< X << std::endl << std::flush;		\
  }

#define MUSIC_LOGR(X) MUSIC_SLOGR (std::cerr, X)

#define MUSIC_LOGRE(X)					\
  {							\
    int _r = MPI::COMM_WORLD.Get_rank ();		\
    char* _e = getenv ("MUSIC_DEBUG_RANK");		\
    if (_e != NULL && atoi (_e) == _r)			\
      {							\
	std::cerr << _r << ": "				\
		  << X << std::endl << std::flush;	\
      }							\
  }

#define MUSIC_LOGBR(C, X)			\
  {						\
    int _r = (C).Get_rank ();			\
    int _n = (C).Get_size ();			\
    for (int _i = 0; _i < _n; ++_i)		\
      {						\
	(C).Barrier ();				\
	if (_i == _r)				\
	  MUSIC_LOGR (X);			\
      }						\
  }

namespace MUSIC {
  static int debug_fd = 0;
}

#define MUSIC_LOGLR(X)							\
  {									\
    if (MUSIC::debug_fd == 0)						\
      MUSIC::debug_fd = open ("/tmp/music_debug",			\
			      O_CREAT | O_WRONLY | O_APPEND,		\
			      00755);					\
    lockf (MUSIC::debug_fd, F_LOCK, 0);					\
    std::ostringstream ostr;						\
    MUSIC_LOGR (X);                                                     \
    MUSIC_SLOGR (ostr, X);						\
    write (MUSIC::debug_fd, ostr.str ().c_str (), ostr.str ().length ()); \
    lockf (MUSIC::debug_fd, F_ULOCK, 0);				\
  }

namespace MUSIC {
  static std::ofstream logfile;
}

#define MUSIC_TLOGR(X)							\
  {									\
    if (!logfile.is_open ())						\
      {									\
	std::ostringstream name;					\
	name << "/tmp/music_log" << MPI::COMM_WORLD.Get_rank ();	\
	logfile.open (name.str ().c_str ());				\
      }									\
    struct timeval tv;							\
    gettimeofday (&tv, NULL);						\
    logfile << std::setw (10) << std::setfill ('0') << tv.tv_sec << std::setw (10) << std::setfill ('0') << tv.tv_usec << ' '; \
    MUSIC_SLOGR (logfile, X);						\
  }

#define MUSIC_LOGX(X)

#else

#define MUSIC_LOG(X)
#define MUSIC_LOGN(N, X)
#define MUSIC_LOG0(X)
#define MUSIC_LOGR(X)
#define MUSIC_LOGRE(X)
#define MUSIC_LOGBR(C, X)
#define MUSIC_LOGLR(X)
#define MUSIC_LOGX(X)
#define MUSIC_TLOGR(X)

#endif

#define MUSIC_DEBUG_HH
#endif

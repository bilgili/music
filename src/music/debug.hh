/*
 *  This file is part of MUSIC.
 *  Copyright (C) 2008 INCF
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
#include <mpi.h>
#include <iostream>

#define MUSIC_LOG(X) (std::cerr << X << std::endl)
#define MUSIC_LOGN(N, X) { if (MPI::COMM_WORLD.Get_rank () == N) std::cerr << X << std::endl; }
#define MUSIC_LOG0(X) MUSIC_LOGN (0, X)
#else
#define MUSIC_LOG(X)
#define MUSIC_LOGN(N, X)
#define MUSIC_LOG0(X)
#endif

#define MUSIC_DEBUG_HH
#endif

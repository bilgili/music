#
# This file is part of MUSIC.
# Copyright (c) 2014 Cajal Blue Brain, BBP/EPFL
#
# MUSIC is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# MUSIC is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

include(CheckFunctionExists)

if(MPI_FOUND)
  # Since these results are cached it's better not to proceed until the MPI
  # libraries have been found.
  set(CMAKE_REQUIRED_INCLUDES ${MPI_INCLUDE_DIRS})
  set(CMAKE_REQUIRED_LIBRARIES ${MPI_CXX_LIBRARIES} ${MPI_C_LIBRARIES})
  check_function_exists(rts_get_personality HAVE_RST_GET_PERSONALITY)
  check_function_exists(ompi_comm_free HAVE_OMPI_COMM_FREE)
endif()

try_compile(HAVE_CXX_MPI_INIT_THREAD ${PROJECT_BINARY_DIR}/config
  ${PROJECT_SOURCE_DIR}/CMake/config/mpi_init_thread.cpp
  CMAKE_FLAGS "-DINCLUDE_DIRECTORIES:STRING=${MPI_CXX_INCLUDE_PATH}"
  LINK_LIBRARIES ${MPI_CXX_LIBRARIES})

if(BGL)
  set(MPI_CXX_COMPILE_FLAGS
    "${MPI_CXX_COMPILE_FLAGS} -qarch=440 -qtune=440 -qhot -qnostrict")
endif()

configure_file(CMake/config/config.h.in ${PROJECT_BINARY_DIR}/config.h)

include_directories(${MPI_C_INCLUDE_PATH}
                    ${PROJECT_BINARY_DIR})

# TODO

# /* Define if this is a Cray XE6 system. */
# #undef CRAY_XE6

# /* Define to 1 if you have MPICH2. */
# #undef HAVE_MPICH2


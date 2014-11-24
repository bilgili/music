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

if(NOT MPI_FOUND)
  message(FATAL_ERROR "MPI is required to compile music")
endif()

set(MPI_IMPLEMENTATIONS BGL MPICH MPICH2 MVAPICH2 OPENMPI CRAY_XE6)
set(MPI_IMPLEMENTATION "MVAPICH2" CACHE STRING "MPI implementation: ${MPI_IMPLEMENTATIONS}")
set_property(CACHE MPI_IMPLEMENTATION PROPERTY STRINGS ${MPI_IMPLEMENTATIONS})

include(CheckFunctionExists)

# Since these results are cached it's better not to proceed until the MPI
# libraries have been found.
set(CMAKE_REQUIRED_INCLUDES ${MPI_INCLUDE_DIRS})
set(CMAKE_REQUIRED_LIBRARIES ${MPI_CXX_LIBRARIES} ${MPI_C_LIBRARIES})

check_function_exists(rts_get_personality HAVE_RST_GET_PERSONALITY)
if(HAVE_RST_GET_PERSONALITY)
  set(MPI_IMPLEMENTATION "BGL")
endif()

check_function_exists(ompi_comm_free HAVE_OMPI_COMM_FREE)
if(HAVE_OMPI_COMM_FREE)
  set(MPI_IMPLEMENTATION "OPENMPI")
endif()

try_compile(HAVE_CXX_MPI_INIT_THREAD ${PROJECT_BINARY_DIR}/config
  ${PROJECT_SOURCE_DIR}/CMake/config/mpi_init_thread.cpp
  CMAKE_FLAGS "-DINCLUDE_DIRECTORIES:STRING=${MPI_CXX_INCLUDE_PATH}"
  LINK_LIBRARIES ${MPI_CXX_LIBRARIES})

if(MPI_IMPLEMENTATION STREQUAL "BGL")
  set(MPI_CXX_COMPILE_FLAGS
    "${MPI_CXX_COMPILE_FLAGS} -qarch=440 -qtune=440 -qhot -qnostrict")
endif()

configure_file(CMake/config/config.h.in ${PROJECT_BINARY_DIR}/config.h)

include_directories(${MPI_C_INCLUDE_PATH}
                    ${PROJECT_BINARY_DIR})


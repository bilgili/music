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

cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(music)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMake
                              ${PROJECT_SOURCE_DIR}/CMake/common)

include(GitExternal)

set(VERSION_MAJOR "1")
set(VERSION_MINOR "0")
set(VERSION_PATCH "7")

macro(remove_flag _FLAG_LIST _FLAG)
  string(REPLACE " " ";" _LIST ${${_FLAG_LIST}})
  list(REMOVE_ITEM _LIST ${_FLAG})
  string(REPLACE ";" " " ${_FLAG_LIST} "${_LIST}")
endmacro()

set(PROJECT_INCLUDE_NAME music)

include(Common)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANG)
  remove_flag(CMAKE_C_FLAGS -Wshadow)
  remove_flag(CMAKE_CXX_FLAGS -Wshadow)
elseif(CMAKE_COMPILER_IS_INTEL)
  remove_flag(CMAKE_CXX_FLAGS -Wshadow)
endif()

include(FindPackages)

include(Configure)

add_subdirectory(mpidep)
add_subdirectory(src)
add_subdirectory(rudeconfig)
add_subdirectory(utils)
add_subdirectory(test)
add_subdirectory(doc)

set(MUSIC_EXCLUDE_LIBRARIES rudeconfig mpidep)
include(PackageConfig)

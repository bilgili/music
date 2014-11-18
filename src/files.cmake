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

include(CheckTypeSize)

macro(CHECK_TYPE_EXISTS TYPE)
  string(REPLACE " " "_" _VAR_NAME ${TYPE})
  string(TOUPPER ${_VAR_NAME} _VAR_NAME)
  check_type_size(${TYPE} SIZEOF_${_VAR_NAME})
  if(SIZEOF_${_VAR_NAME})
    set(MUSIC_HAVE_${_VAR_NAME} 1)
  else()
    set(MUSIC_HAVE_${_VAR_NAME} 0)
  endif()
endmacro()

check_type_exists("size_t")
check_type_exists("long long")

configure_file(music/music-config.hh.in
               ${PROJECT_BINARY_DIR}/music/music-config.hh
               @ONLY)

set(MUSIC_SOURCES
  ${COMMON_SOURCES}
  BIFO.cc
  FIBO.cc
  application_map.cc
  array_data.cc
  clock.cc
  collector.cc
  configuration.cc
  connection.cc
  connectivity.cc
  connector.cc
  distributor.cc
  error.cc
  event_router.cc
  index_map.cc
  index_map_factory.cc
  ioutils.cc
  linear_index.cc
  parse.cc
  permutation_index.cc
  port.cc
  predict_rank.cc
  runtime.cc
  sampler.cc
  setup.cc
  spatial.cc
  subconnector.cc
  synchronizer.cc
  temporal.cc
  version.cc
  )

set(MUSIC_C_SOURCES
  music-c.cc
  music-c-c.c
  predict_rank-c.cc
  )

set(MUSIC_HEADERS
  music/BIFO.hh
  music/FIBO.hh
  music/application_map.hh
  music/array_data.hh
  music/clock.hh
  music/collector.hh
  music/communication.hh
  music/configuration.hh
  music/connection.hh
  music/connectivity.hh
  music/connector.hh
  music/data_map.hh
  music/debug.hh
  music/distributor.hh
  music/error.hh
  music/event_router.hh
  music/index_map.hh
  music/index_map_factory.hh
  music/interval.hh
  music/interval_tree.hh
  music/ioutils.hh
  music/linear_index.hh
  music/message.hh
  music/parse.hh
  music/permutation_index.hh
  music/port.hh
  music/predict_rank.hh
  music/runtime.hh
  music/sampler.hh
  music/setup.hh
  music/spatial.hh
  music/subconnector.hh
  music/synchronizer.hh
  music/temporal.hh
  music/version.hh
  )

set(MUSIC_C_HEADERS
  music-c.h)

set(MUSIC_PUBLIC_HEADERS
  ${COMMON_INDLUDES}
  ${CMAKE_SOURCE_DIR}/src/music/BIFO.hh
  ${CMAKE_SOURCE_DIR}/src/music/FIBO.hh
  ${CMAKE_SOURCE_DIR}/src/music/application_map.hh
  ${CMAKE_SOURCE_DIR}/src/music/array_data.hh
  ${CMAKE_SOURCE_DIR}/src/music/clock.hh
  ${CMAKE_SOURCE_DIR}/src/music/collector.hh
  ${CMAKE_SOURCE_DIR}/src/music/communication.hh
  ${CMAKE_SOURCE_DIR}/src/music/configuration.hh
  ${CMAKE_SOURCE_DIR}/src/music/connectivity.hh
  ${CMAKE_SOURCE_DIR}/src/music/connector.hh
  ${CMAKE_SOURCE_DIR}/src/music/connection.hh
  ${CMAKE_SOURCE_DIR}/src/music/cont_data.hh
  ${CMAKE_SOURCE_DIR}/src/music/distributor.hh
  ${CMAKE_SOURCE_DIR}/src/music/event.hh
  ${CMAKE_SOURCE_DIR}/src/music/event_router.hh
  ${CMAKE_SOURCE_DIR}/src/music/data_map.hh
  ${CMAKE_SOURCE_DIR}/src/music/debug.hh
  ${CMAKE_SOURCE_DIR}/src/music/error.hh
  ${CMAKE_SOURCE_DIR}/src/music/linear_index.hh
  ${CMAKE_SOURCE_DIR}/src/music/index_map.hh
  ${CMAKE_SOURCE_DIR}/src/music/index_map_factory.hh
  ${CMAKE_SOURCE_DIR}/src/music/interval.hh
  ${CMAKE_SOURCE_DIR}/src/music/interval_tree.hh
  ${CMAKE_SOURCE_DIR}/src/music/ioutils.hh
  ${CMAKE_SOURCE_DIR}/src/music/message.hh
  ${PROJECT_BINARY_DIR}/music/music-config.hh
  ${CMAKE_SOURCE_DIR}/src/music/port.hh
  ${CMAKE_SOURCE_DIR}/src/music/permutation_index.hh
  ${CMAKE_SOURCE_DIR}/src/music/runtime.hh
  ${CMAKE_SOURCE_DIR}/src/music/setup.hh
  ${CMAKE_SOURCE_DIR}/src/music/sampler.hh
  ${CMAKE_SOURCE_DIR}/src/music/spatial.hh
  ${CMAKE_SOURCE_DIR}/src/music/subconnector.hh
  ${CMAKE_SOURCE_DIR}/src/music/synchronizer.hh
  ${CMAKE_SOURCE_DIR}/src/music/temporal.hh
  ${CMAKE_SOURCE_DIR}/src/music/version.hh
  )


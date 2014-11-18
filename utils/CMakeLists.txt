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

include_directories(${PROJECT_SOURCE_DIR}/src)

set(MUSICAPP_HEADERS application_mapper.hh)
set(MUSICAPP_SOURCES music.cc application_mapper.cc)
set(MUSICAPP_LINK_LIBRARIES ${MPI_CXX_LIBRARIES} music mpidep rudeconfig)
common_application(musicapp)
set_target_properties(musicapp PROPERTIES OUTPUT_NAME music)

set(EVENTSOURCE_HEADERS datafile.h)
set(EVENTSOURCE_SOURCES eventsource.cc datafile.cc)
set(EVENTSOURCE_LINK_LIBRARIES ${MPI_CXX_LIBRARIES} music)
common_application(eventsource)

set(EVENTSINK_SOURCES eventsink.cc)
set(EVENTSINK_LINK_LIBRARIES ${MPI_CXX_LIBRARIES} music)
common_application(eventsink)

set(EVENTSELECT_SOURCES eventselect.cc)
set(EVENTSELECT_LINK_LIBRARIES ${MPI_CXX_LIBRARIES} music)
common_application(eventselect)

set(EVENTGENERATOR_SOURCES eventgenerator.cc)
set(EVENTGENERATOR_LINK_LIBRARIES ${MPI_CXX_LIBRARIES} music)
common_application(eventgenerator)

set(EVENTCOUNTER_SOURCES eventcounter.cc)
set(EVENTCOUNTER_LINK_LIBRARIES ${MPI_CXX_LIBRARIES} music)
common_application(eventcounter)

if(GLUT_FOUND AND OPENGL_FOUND AND THREADS_FOUND)
  set(VIEWEVENTS_HEADERS VisualiseNeurons.h)
  set(VIEWEVENTS_SOURCES viewevents.cpp VisualiseNeurons.cpp)
  set(VIEWEVENTS_LINK_LIBRARIES music
    ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
  common_application(viewevents)
endif()

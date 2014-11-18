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

include(files.cmake)

add_definitions(-DBUILDING_MUSIC_LIBRARY)

include_directories(${PROJECT_SOURCE_DIR}/src)

set(MUSIC_LINK_LIBRARIES ${MPI_LIBRARIES} mpidep)
common_library(music)

set(MUSIC_C_LINK_LIBRARIES music)
common_library(music_c)
set_target_properties(music_c PROPERTIES OUTPUT_NAME "music-c")

# The following haders are not handled gracefully by common_library because
# they need to be installed outside include/music
# The original autoconf scripts install also predict-rank-c.h. It has been
# excluded because it's C++ countepart is not exported, which makes it look
# like a private header.
install(FILES music-c.h music.hh
        DESTINATION include)

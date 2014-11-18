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

set(RUDECONFIG_SOURCES
  src/AbstractData.cpp
  src/AbstractOrganiser.cpp
  src/AbstractParser.cpp
  src/AbstractWriter.cpp
  src/Base64Encoder.cpp
  src/Comment.cpp
  src/ConfigImpl.cpp
  src/DataLine.cpp
  src/File.cpp
  src/KeyValue.cpp
  src/ParserJuly2004.cpp
  src/RealOrganiser.cpp
  src/Section.cpp
  src/SourceDest.cpp
  src/WhiteSpace.cpp
  src/Writer.cpp
  src/config.cpp
  )

set(RUDECONFIG_HEADERS
  src/AbstractData.h
  src/AbstractOrganiser.h
  src/AbstractParser.h
  src/AbstractWriter.h
  src/Base64Encoder.h
  src/Comment.h
  src/ConfigImpl.h
  src/DataLine.h
  src/File.h
  src/KeyValue.h
  src/ParserJuly2004.h
  src/RealOrganiser.h
  src/Section.h
  src/SourceDest.h
  src/WhiteSpace.h
  src/Writer.h
  src/config.h
  )



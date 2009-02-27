/*
 *  This file is part of MUSIC.
 *  Copyright (C) 2009 INCF
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

//*fixme* switch name to Interpolator?

#ifndef MUSIC_SAMPLER_HH

#include <music/data_map.hh>

namespace MUSIC {

  class Sampler {
    DataMap* dataMap_;
    DataMap* interpolationDataMap_;
    bool hasSampled;
    ContDataT* prevSample_;
    ContDataT* sample_;
    ContDataT* interpolationData_;
    void swapBuffers (ContDataT*& b1, ContDataT*& b2);
  public:
    Sampler ();
    ~Sampler ();
    void configure (DataMap* dataMap);
    DataMap* dataMap () { return dataMap_; }
    // this class manages one single copy of the interpolation DataMap
    DataMap* interpolationDataMap ();
    void newSample ();
    void sample ();
    ContDataT* insert ();
    void interpolate (double interpolationCoefficient);
    void interpolateToApplication (double interpolationCoefficient);
  };

}

#define MUSIC_SAMPLER_HH
#endif

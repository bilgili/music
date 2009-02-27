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

#include "music/sampler.hh"

namespace MUSIC {

  Sampler::Sampler ()
    : dataMap_ (0), interpolationDataMap_ (0)
  {
  }


  Sampler::~Sampler ()
  {
    if (dataMap_ != 0)
      delete dataMap_;
    if (interpolationDataMap_ != 0)
      delete interpolationDataMap_;
  }
  
  
  void
  Sampler::configure (DataMap* dataMap)
  {
    dataMap_ = dataMap->copy ();
  }

  
  DataMap*
  Sampler::interpolationDataMap ()
  {
  }


  void
  Sampler::newSample ()
  {
    hasSampled = false;
  }


  void
  Sampler::sample ()
  {
    if (!hasSampled)
      {
	hasSampled = true;
      }
  }

  
  ContDataT*
  Sampler::insert ()
  {
  }

  
  void
  Sampler::swapBuffers (ContDataT*& b1, ContDataT*& b2)
  {
    ContDataT* tmp;
    tmp = b1;
    b1 = b2;
    b2 = tmp;
  }


  void
  Sampler::interpolate (double interpolationCoefficient)
  {
    
  }
  
  void
  Sampler::interpolateToApplication (double interpolationCoefficient)
  {
    
  }
  

}

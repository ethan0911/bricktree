// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

#include "ChomboInput.h"

namespace ospray {
  namespace amr {

    /*! this is how we store Chombo bricks internally */
    struct ChomboInternal {
      ChomboInternal(const ChomboInput *input);

      struct Brick : public ChomboData::BrickInfo {
        /* world bounds, including entire cells. ie, a 4^3 root brick
           at (0,0,0) would have bounds [(0,0,0)-(4,4,4)] (as opposed
           to the 'box' value, see above!) */
        box3f bounds;
        // pointer to the actual data values stored in this brick
        float *value;
        // dimensions of this box's data
        vec3i dims;
        // scale factor from grid space to world space (ie,1.f/cellWidth)
        float gridToWorldScale;
        
        // rcp(bounds.upper-bounds.lower);
        vec3f bounds_scale;
        // dimensions, in float
        vec3f f_dims;
      };
      
      size_t numBricks;
      Brick *brickArray;
    };

  } // ::ospray::amr
} // ::ospray

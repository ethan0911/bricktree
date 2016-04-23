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

// amr base
#include "../amr/Array3D.h"

namespace ospray {
  namespace amr {

    /*! this structure defines only the format of the INPUT of chombo
        data - ie, what we get from the scene graph or applicatoin */
    struct ChomboInput {
      ChomboInput(const OSPData brickInfoData, 
                  const OSPData brickDataData);

      /*! this is how an app _specifies_ a brick (or better, the array
        of bricks); the brick data is specified through a separate
        array of data buffers (one data buffer per brick) */
      struct Brick {
        /*! bounding box of integer coordinates of cells. note that
          this EXCLUDES the width of the rightmost cell: ie, a 4^3
          box at root level pos (0,0,0) would have a _box_ of
          [(0,0,0)-(3,3,3)] (because 3,3,3 is the highest valid
          coordinate in this box!), while its bounds would be
          [(0,0,0)-(4,4,4)]. Make sure to NOT use box.size() for the
          grid dimensions, since this will always be one lower than
          the dims of the grid.... */
        box3i box;
        //! level this brick is at
        int   level;
        // width of each cell in this level
        float cellWidth;
      };
      
      const Brick  brickInfo[];
      const float *brickData[];
      const size_t numBricks;
    };

  } // ::ospray::amr
} // ::ospray

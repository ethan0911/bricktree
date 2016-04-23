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

#include "ChomboInternal.h"

namespace ospray {
  namespace amr {

      ChomboInternal::ChomboInternal(const ChomboInput *input)
        : brick(new Brick[input->numBricks]),
          numBricks(input->numBricks)
      {
        assert(brickInfoData);
        numBricks = brickInfoData->numBytes / sizeof(BrickInfo);
        cout << "#osp:chom: making bricks - found " << numBricks << " brick infos" << endl;
        brickArray = new Brick[numBricks];
        
        Data **allBricksData = (Data **)brickDataData->data;

      for (int i=0;i<numBricks;i++) {
        Brick *brick = brickArray+i;
        BrickInfo *info = (BrickInfo*)brickInfoData->data + i;
        Data      *thisBrickData = allBricksData[i];
        // copy values
        brick->box = info->box;
        brick->level = info->level;
        brick->cellWidth = info->cellWidth;

        brick->value = (float*)thisBrickData->data;
        brick->dims  = brick->box.size() + vec3i(1);
        brick->gridToWorldScale = 1.f/brick->cellWidth;
        brick->bounds = box3f(vec3f(brick->box.lower) * brick->cellWidth, 
                              vec3f(brick->box.upper+vec3i(1)) * brick->cellWidth);
        brick->bounds_scale = rcp(brick->bounds.size());
        brick->f_dims = vec3f(brick->dims);
        
      }

  } // ::ospray::amr
} // ::ospray

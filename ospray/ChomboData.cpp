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

// amr base
#include "ChomboData.h"

namespace ospray {
  namespace amr {

    /*! initialize an internal brick representation from input
        brickinfo and corresponding input data pointer */
    ChomboData::Brick::Brick(const BrickInfo &info, const float *data)
    {
      this->box = info.box;
      this->level = info.level;
      this->cellWidth = info.cellWidth;
      
      this->value = data;
      this->dims  = this->box.size() + vec3i(1);
      this->gridToWorldScale = 1.f/this->cellWidth;
      this->bounds = box3f(vec3f(this->box.lower) * this->cellWidth, 
                           vec3f(this->box.upper+vec3i(1)) * this->cellWidth);
      this->bounds_scale = rcp(this->bounds.size());
      this->f_dims = vec3f(this->dims);
    }

    inline size_t getNumBricks(const Data &brickInfoData)
    {
      return brickInfoData.numBytes / sizeof(ChomboData::BrickInfo);
    }
    
    /*! this structure defines only the format of the INPUT of chombo
      data - ie, what we get from the scene graph or applicatoin */
    ChomboData::ChomboData(const Data &brickInfoData, 
                           const Data &brickDataData)
      : numBricks(getNumBricks(brickInfoData)),
        brick(NULL)
    {
      std::cout << "#osp:chom: found " << numBricks << " brick infos" << std::endl;
      brick = new const Brick *[numBricks];
      const BrickInfo *brickInfo = (const BrickInfo *)brickInfoData.data;
      const Data **allBricksData = (const Data **)brickDataData.data;
      for (int i=0;i<numBricks;i++) 
        brick[i] = new Brick(brickInfo[i],(const float*)allBricksData[i]->data);
    }

  } // ::ospray::amr
} // ::ospray


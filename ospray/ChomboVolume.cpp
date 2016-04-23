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

#include "ChomboVolume.h"
// ospray
#include "ospray/common/Model.h"
#include "ospray/common/Data.h"
#include "ospray/transferFunction/TransferFunction.h"
// ispc exports
#include "ChomboVolume_ispc.h"
// stl
#include <set>
#include <map>

namespace ospray {
  namespace amr {

    ChomboVolume::ChomboVolume()
      : Volume(), accel(NULL), data(NULL)
    {
      ispcEquivalent = ispc::ChomboVolume_create(this);
    }

      /*! Copy voxels into the volume at the given index (non-zero
        return value indicates success). */
    int ChomboVolume::setRegion(const void *source, const vec3i &index, const vec3i &count)
    {
      FATAL("'setRegion()' doesn't make sense for AMR volumes; "
            "they can only be set from existing data");
    }

    //! Allocate storage and populate the volume.
    void ChomboVolume::commit()
    {
      std::cout << "#osp:amr: creating chombo volume" << std::endl;
      
      brickInfoData = getParamData("brickInfo");
      assert(brickInfoData);
      assert(brickInfoData->data);

      brickDataData = getParamData("brickData");
      assert(brickDataData);
      assert(brickDataData->data);
      
      assert(data == NULL);
      data = new ChomboData(*brickInfoData,*brickDataData);
      assert(accel == NULL);
      accel = new ChomboAccel(data);

      Ref<TransferFunction> xf = (TransferFunction*)getParamObject("transferFunction");
      assert(xf);

      float finestLevelCellWidth = data->brick[0]->cellWidth;
      box3i rootLevelBox = empty;
      for (int i=0;i<data->numBricks;i++) {
        if (data->brick[i]->level == 0)
          rootLevelBox.extend(data->brick[i]->box);
        finestLevelCellWidth = min(finestLevelCellWidth,data->brick[i]->cellWidth);
      }
      vec3i rootGridDims = rootLevelBox.size()+vec3i(1);
      std::cout << "#osp:chom: found root level dimensions of " << rootGridDims << std::endl;

      int method = AMR_FINEST;
      const char *methodStringFromEnv = getenv("OSPRAY_AMR_METHOD");
      if (methodStringFromEnv) {
        const std::string methodString = methodStringFromEnv;
        std::cout << "-------------------------------------------------------" << std::endl;
        std::cout << "-------------------------------------------------------" << std::endl;
        std::cout << "-------------------------------------------------------" << std::endl;
        std::cout << "FOUND 'OSPRAY_AMR_METHOD' env-var ('" << methodString << "')" << std::endl;
        std::cout << "-------------------------------------------------------" << std::endl;
        std::cout << "-------------------------------------------------------" << std::endl;
        std::cout << "-------------------------------------------------------" << std::endl;
        if (methodString == "AMR_NEAREST" ||
            methodString == "nearest")
          method = AMR_NEAREST;
        else if (methodString == "AMR_FINEST" ||
                 methodString == "finest")
          method = AMR_FINEST;
        else if (methodString == "AMR_OCT_SIMPLE" ||
                 methodString == "oct-simple")
          method = AMR_OCT_SIMPLE;
        else if (methodString == "AMR_BLEND" ||
                 methodString == "blend")
          method = AMR_BLEND;
        else 
          throw std::runtime_error("cannot parse ospray_amr_method '"+methodString+"'");
      }
      ispc::ChomboVolume_set(getIE(),
                             method,
                             xf->getIE(),
                             (ispc::vec3i&)rootGridDims,
                             &accel->node[0],
                             accel->node.size(),
                             &accel->leaf[0],
                             accel->leaf.size(),
                             finestLevelCellWidth);
      std::cout << "#osp:chom: done building chombo volume" << std::endl;
    }

    OSP_REGISTER_VOLUME(ChomboVolume,ChomboVolume);
    OSP_REGISTER_VOLUME(ChomboVolume,chombo_volume);
  }
} // ::ospray


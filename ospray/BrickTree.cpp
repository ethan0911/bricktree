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

#include "BrickTree.h"
// ospray
#include "ospray/common/Model.h"
#include "ospray/common/Data.h"
#include "ospray/transferFunction/TransferFunction.h"
// ispc exports
//#include "BrickTree_ispc.h"
// stl
#include <set>
#include <map>
#include <stack>

namespace ospray {
  namespace amr {
    using std::endl;
    using std::cout;
    using std::ostream;
    using std::flush;

    BrickTreeVolume::BrickTreeVolume()
      : Volume(), rootGridDims(-1), voxelType(OSP_FLOAT)
    {
      // ispcEquivalent = ispc::BrickTreeVolume_create(this);
    }
    
    //! Allocate storage and populate the volume.
    void BrickTreeVolume::commit()
    {
      // right now we only support floats ...
      this->voxelType = OSP_FLOAT;

      cout << "#osp:amr: BrickTreeVolume::commit()" << endl;
      firstIndexBrickOfTreeData   = getParamData("firstIndexBrickOfTree");
      firstDataBrickOfTreeData   = getParamData("firstDataBrickOfTree");
      dataBrickData  = getParamData("dataBrickData");
      indexBrickData = getParamData("indexBrickData");
      brickInfoData  = getParamData("brickInfoData");
      rootGridDims   = getParam3i("rootGridDims",vec3i(-1));
      validFractionOfRootGrid = getParam3f("validFractionOfRootGrid",vec3f(0.f));
      int maxLevel = getParam1i("maxLevel",0);
      if (maxLevel < 1)
        throw std::runtime_error("BrickTreeVolume: maxLevel not specified");
      float finestCellWidth = 1.f;
      for (int i=0;i<maxLevel;i++)
        finestCellWidth *= .25f;

      Ref<TransferFunction> xf = (TransferFunction*)getParamObject("transferFunction");

      // ispc::BrickTreeVolume_set(getIE(),
      //                       xf->getIE(),
      //                       (ispc::vec3i &)rootGridDims,
      //                       (ispc::vec3f &)validFractionOfRootGrid,
      //                       dataBrickData->data,
      //                       indexBrickData->data,
      //                       brickInfoData->data,
      //                       firstDataBrickOfTreeData->data,
      //                       firstIndexBrickOfTreeData->data,
      //                       finestCellWidth
      //                       );
    }

    OSP_REGISTER_VOLUME(BrickTreeVolume,BrickTreeVolume);

  }
} // ::ospray


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

#include "MultiSum.h"
// ospray
#include "ospray/common/Model.h"
#include "ospray/common/Data.h"
#include "ospray/transferFunction/TransferFunction.h"
#include "../amr/Sumerian.h"
// ispc exports
#include "MultiSum_ispc.h"
// stl
#include <set>
#include <map>

namespace ospray {
  namespace amr {
    using std::endl;
    using std::cout;
    using std::ostream;
    using std::flush;

    MultiSumAMRVolume::MultiSumAMRVolume()
      : Volume(), rootGridDims(-1), voxelType(OSP_FLOAT)
    {
      ispcEquivalent = ispc::MultiSumAMRVolume_create(this);
    }
    
    //! Allocate storage and populate the volume.
    void MultiSumAMRVolume::commit()
    {
      // right now we only support floats ...
      this->voxelType = OSP_FLOAT;

      cout << "#osp:amr: MultiSumAMRVolume::commit()" << endl;
      rootCellData   = getParamData("rootCellData");
      dataBlockData  = getParamData("dataBlockData");
      indexBlockData = getParamData("indexBlockData");
      blockInfoData  = getParamData("blockInfoData");
      rootGridDims   = getParam3i("rootGridDims",vec3i(-1));
      validFractionOfRootGrid = getParam3f("validFractionOfRootGrid",vec3f(0.f));

      PRINT(rootGridDims);
      PRINT(validFractionOfRootGrid);

      Ref<TransferFunction> xf = (TransferFunction*)getParamObject("transferFunction");


      PRINT(*(Sumerian::IndexBlock *)indexBlockData->data);
      PRINT(*(Sumerian::DataBlock *)dataBlockData->data);
      PRINT(*(int*)blockInfoData->data);
      PRINT(*(int*)rootCellData->data);

      ispc::MultiSumAMRVolume_set(getIE(),
                                  xf->getIE(),
                                  (ispc::vec3i &)rootGridDims,
                                  (ispc::vec3f &)validFractionOfRootGrid,
                                  indexBlockData->data,
                                  dataBlockData->data,
                                  blockInfoData->data,
                                  rootCellData->data);
    }

    OSP_REGISTER_VOLUME(MultiSumAMRVolume,MultiSumAMRVolume);

  }
} // ::ospray

